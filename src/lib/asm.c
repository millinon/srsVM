#include <ctype.h>   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "srsvm/asm.h"
#include "srsvm/debug.h"

#if defined(SRSVM_BUILD_TARGET_WINDOWS)
char* strndup(char* str, size_t len)
{
	char* dup = (char*)malloc(len + 1);
	if (dup) {
		strncpy(dup, str, len);
		dup[len] = '\0';
	}
	return dup;
}
#endif

static inline void report_warning(srsvm_assembly_program *program, const char* filename, const unsigned long line_number, const char* message, void* config)
{
    if(program != NULL && filename != NULL && message != NULL){
        if(program->on_warning != NULL){
            program->on_warning(filename, line_number, message, config);
        }
    }
}

static inline void report_error(srsvm_assembly_program *program, const char* filename, const unsigned long line_number, const char* message, void* config)
{
    if(program != NULL && filename != NULL && message != NULL){
        if(program->on_error != NULL){
            program->on_error(filename, line_number, message, config);
        }
    }
}

srsvm_assembly_line *srsvm_asm_line_alloc(void)
{
    srsvm_assembly_line *line = malloc(sizeof(srsvm_assembly_line));

    if(line != NULL){
        memset(line, 0, sizeof(srsvm_assembly_line));
    }

    return line;
}
void srsvm_asm_line_free(srsvm_assembly_line *line)
{
    if(line != NULL){
        if(line->next != NULL){
            srsvm_asm_line_free(line->next);
        }

        free(line);
    }
}

srsvm_assembly_program *srsvm_asm_program_alloc(srsvm_assembler_message_report_func on_error, srsvm_assembler_message_report_func on_warning, void* io_config)
{
    srsvm_assembly_program *program = malloc(sizeof(srsvm_assembly_program));

    if(program != NULL){
        memset(program, 0, sizeof(srsvm_assembly_program));

        if((program->label_map = srsvm_string_map_alloc(false)) == NULL){
            goto error_cleanup;
        } else if((program->mod_map = srsvm_string_map_alloc(false)) == NULL){
            goto error_cleanup;
        } else if((program->reg_map = srsvm_string_map_alloc(false)) == NULL){
            goto error_cleanup;
        } else if((program->const_map = srsvm_string_map_alloc(false)) == NULL){
            goto error_cleanup;
        } else if((program->opcode_map = srsvm_opcode_map_alloc()) == NULL){
            goto error_cleanup;
        } else if(! load_builtin_opcodes(program->opcode_map)) {
            goto error_cleanup;
        } else {
#define xstr(a) str(a)
#define str(a) #a
#define LOAD_BUILTIN(op) do { \
            if((program->builtin_##op = opcode_lookup_by_name(program->opcode_map, xstr(op))) == NULL){ \
                if(on_error != NULL) on_error("<init>", 0, "failed to load built-in opcode " xstr(op), io_config); \
                goto error_cleanup; \
            } } while(0)

    LOAD_BUILTIN(LOAD_CONST);
    LOAD_BUILTIN(CMOD_LOAD);
    LOAD_BUILTIN(CMOD_UNLOAD);
    LOAD_BUILTIN(MOD_UNLOAD_ALL);
    LOAD_BUILTIN(CMOD_OP);
    LOAD_BUILTIN(NOP);
    LOAD_BUILTIN(JMP);
    LOAD_BUILTIN(JMP_IF);
    LOAD_BUILTIN(JMP_ERR);
    LOAD_BUILTIN(CJMP_FORWARD);
    LOAD_BUILTIN(CJMP_FORWARD_IF);
    LOAD_BUILTIN(CJMP_FORWARD_ERR);
    LOAD_BUILTIN(CJMP_BACK);
    LOAD_BUILTIN(CJMP_BACK_IF);
    LOAD_BUILTIN(CJMP_BACK_ERR);
#undef xstr
#undef str
#undef LOAD_BUILTIN
        }

        //strncpy(program->input_filename, input_filename, sizeof(program->input_filename));
        program->on_error = on_error;
        program->on_warning = on_warning;

        program->io_config = io_config;
    }

    return program;

error_cleanup:
    if(program != NULL){
        srsvm_asm_program_free(program);
    }

    return NULL;
}

void srsvm_asm_program_free(srsvm_assembly_program *program)
{
    if(program != NULL){
        if(program->constants != NULL){
            free(program->constants);
        }
        if(program->registers != NULL){
            free(program->registers);
        }
        if(program->label_map != NULL){
            srsvm_string_map_free(program->label_map, true);
        }
        if(program->mod_map != NULL){
            srsvm_string_map_free(program->mod_map, false);
        }
        if(program->reg_map != NULL){
            srsvm_string_map_free(program->reg_map, true);
        }
        if(program->const_map != NULL){
            srsvm_string_map_free(program->const_map, false);
        }
        if(program->module_search_path != NULL){
            for(size_t i = 0; program->module_search_path[i] != NULL; i++){
                free(program->module_search_path[i]);
            }
            free(program->module_search_path);
        }

        free(program);
    }
}

typedef enum
{
    LEX_INIT,
    LEX_LABEL,
    LEX_OPCODE,
    LEX_ARG,
    LEX_COMMENT,
    LEX_FINISH,
    LEX_ERROR
} line_lexer_state;

static char* unescape(const char* str_value)
{
    char* raw = NULL;

    if(str_value != NULL){
        size_t str_len = strlen(str_value);

        if((raw = malloc((str_len-1) * sizeof(char))) != NULL){
            memset(raw, 0, (str_len-1) * sizeof(char));

            size_t r = 0;
            for(size_t i = 1; i < str_len-1; i++){
                if(str_value[i] == '\\'){
                    if(i == str_len-2){
                        // truncated escape
                        free(raw);
                        raw = NULL;
                        break;
                    } else {
                        char esc = str_value[++i];

                        switch(esc){
                            case 'a':
                                raw[r++] = '\a';
                                break;
                            case 'b':
                                raw[r++] = '\b';
                                break;
                            case 'f':
                                raw[r++] = '\f';
                                break;
                            case 'n':
                                raw[r++] = '\n';
                                break;
                            case 'r':
                                raw[r++] = '\r';
                                break;
                            case 't':
                                raw[r++] = '\t';
                                break;
                            case 'v':
                                raw[r++] = '\v';
                                break;
                            case '\\':
                                raw[r++] = '\\';
                                break;
                            case '\'':
                                raw[r++] = '\'';
                                break;
                            case '"':
                                raw[r++] = '"';
                                break;
                            case '\?':
                                raw[r++] = '\?';
                                break;

                                // case 'n':
                                // case 'x':
                                // case 'u':
                                // case 'U':

                            default:
                                // unsupported escape
                                free(raw);
                                raw = NULL;
                                break;
                        }
                    }
                } else {
                    raw[r++] = str_value[i];
                }
            }
        }
    }

    return raw;
}

static bool parse_unsigned_hex_word(const char* str, srsvm_word *value)
{
    bool success = false;

    if(str != NULL){
#if WORD_SIZE == 128
        uint64_t upper = 0, lower = 0;
        size_t input_len = strlen(str);

        if(strncmp(str, "0x", 2) == 0){
            str += 2;
            input_len -= 2;
        }

        if(input_len <= 32){
            if(sscanf(str, "%" SCNx64, &lower) == 1){
                if(value != NULL){
                    *value = lower;
                }

                success = true;
            }
        } else {
            char lower_buf[33] = { 0 };
            char upper_buf[33] = { 0 };

            size_t upper_len = input_len - 32;

            strncpy(lower_buf, str + upper_len, 32);
            strncpy(upper_buf, str, upper_len);

            if(sscanf(lower_buf, "%" SCNx64, &lower) == 1 && sscanf(upper_buf, "%" SCNx64, &upper) == 1){
                if(value != NULL){
                    *value = ((((unsigned __int128) upper) << 64) | ((unsigned __int128) lower));
                }

                success = true;
            }
        }
#else
        if(sscanf(str, SCAN_WORD_HEX, value) == 1){
            success = true;
        }
#endif
    }

    return success;
}

static bool parse_signed_hex_word(const char* str, srsvm_ptr_offset *value)
{
    bool success = false;

    if(str != NULL){
        size_t input_len = strlen(str);

        srsvm_word unsigned_value;
        srsvm_ptr_offset signed_value;
        bool is_negative = false;
        bool unsigned_parse_success = false;


        if(input_len >= 1 && str[0] == '-'){
            is_negative = true;

            str++;
        }

        if(parse_unsigned_hex_word(str, &unsigned_value)){
            unsigned_parse_success = true;

            if(is_negative){
                signed_value = (srsvm_ptr_offset) -1 * unsigned_value;
            } else {
                signed_value = unsigned_value;
            }
        }

        if(unsigned_parse_success && signed_value >= SRSVM_MIN_PTR_OFF && signed_value <= SRSVM_MAX_PTR_OFF){
            success = true;

            if(value != NULL){
                *value = signed_value;
            }
        }
    }

    return success;
}

static bool has_suffix(const char* str, const char* suffix, bool case_insensitive)
{
    bool match = false;

    if(str != NULL && suffix != NULL){
        size_t str_len = strlen(str);
        size_t suffix_len = strlen(suffix);

        if(suffix_len <= str_len){
            if(case_insensitive){
                match = srsvm_strncasecmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
            } else {
                match = strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
            }
        }
    }

    return match;
}

static srsvm_constant_value* parse_const(const char* str_value)
{
    dbg_printf("attempting to parse constant '%s'", str_value);

    srsvm_constant_value *value = NULL;

    if(str_value != NULL && strlen(str_value) > 0){
        size_t str_len = strlen(str_value);
        char* raw = NULL;

        if(str_len > 0)
        {
            if(str_len >= 2){
                if(str_value[0] == '"' && str_value[str_len-1] == '"'){
                    raw = unescape(str_value);

                    value = srsvm_const_alloc(SRSVM_TYPE_STR);
                    value->str = srsvm_strdup(raw);
                    value->str_len = str_len-2;

                    dbg_printf("found string constant: %s", str_value);

                } else if(srsvm_strncasecmp(str_value, "TRUE", strlen(str_value)) == 0){
                    value = srsvm_const_alloc(SRSVM_TYPE_BIT);
                    value->bit = true;
                } else if(srsvm_strncasecmp(str_value, "FALSE", strlen(str_value)) == 0){
                    value = srsvm_const_alloc(SRSVM_TYPE_BIT);
                    value->bit = false;
                } else if(has_suffix(str_value, "%ptr_off", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_PTR_OFFSET);

                    raw = strndup(str_value, str_len - strlen("%ptr_off"));

                    if(!parse_signed_hex_word(raw, &value->ptr_offset)){
                        srsvm_const_free(value);
                        value = NULL;
                    }
                } else if(has_suffix(str_value, "%ptr", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_PTR);

                    raw = strndup(str_value, str_len - strlen("%ptr"));

                    if(! parse_unsigned_hex_word(raw, &value->ptr)){
                        srsvm_const_free(value);
                        value = NULL;
                    }
                } else if(has_suffix(str_value, "%u8", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_U8);

                    raw = strndup(str_value, str_len - strlen("%u8"));

                    if(sscanf(raw, "0x%" SCNx8, &value->u8) != 1 &&
                            sscanf(raw, "%" SCNu8, &value->u8) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    }
                } else if(has_suffix(str_value, "%i8", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_I8);

                    raw = strndup(str_value, str_len - strlen("%i8"));

                    if(sscanf(raw, "%" SCNi8, &value->i8) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    }

                } else if(has_suffix(str_value, "%u16", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_U16);

                    raw = strndup(str_value, str_len - strlen("%u16"));

                    if(sscanf(raw, "0x%" SCNx16, &value->u16) != 1 &&
                            sscanf(raw, "%" SCNu16, &value->u16) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    } else {
                        dbg_printf("got U16: %" PRIx16, value->u16);
                    }

                } else if(has_suffix(str_value, "%i16", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_I16);

                    raw = strndup(str_value, str_len - strlen("%i16"));

                    if(sscanf(raw, "%" SCNi16, &value->i16) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    }
#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
                } else if(has_suffix(str_value, "%u32", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_U32);

                    raw = strndup(str_value, str_len - strlen("%u32"));

                    if(sscanf(raw, "0x%" SCNx32, &value->u32) != 1 &&
                            sscanf(raw, "%" SCNu32, &value->u32) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    }
                } else if(has_suffix(str_value, "%i32", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_I32);

                    raw = strndup(str_value, str_len - strlen("%i32"));

                    if(sscanf(raw, "%" SCNi32, &value->i32) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    }
                } else if(has_suffix(str_value, "%f32", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_F32);

                    raw = strndup(str_value, str_len - strlen("%f32"));

                    if(sscanf(raw, "%f", &value->f32) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    }
#endif
#if WORD_SIZE == 64 || WORD_SIZE == 128
                } else if(has_suffix(str_value, "%u64", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_U64);

                    raw = strndup(str_value, str_len - strlen("%u64"));

                    if(sscanf(raw, "0x%" SCNx64, &value->u64) != 1 &&
                            sscanf(raw, "%" SCNu64, &value->u64) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    }
                } else if(has_suffix(str_value, "%i64", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_I64);

                    raw = strndup(str_value, str_len - strlen("%i64"));

                    if(sscanf(raw, "%" SCNi64, &value->i64) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    }
                } else if(has_suffix(str_value, "%f64", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_F64);

                    raw = strndup(str_value, str_len - strlen("%f64"));

                    if(sscanf(raw, "%lf", &value->f64) != 1){
                        srsvm_const_free(value);
                        value = NULL;
                    }
#endif
#if WORD_SIZE == 128
                } else if(has_suffix(str_value, "%u128", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_U128);

                    raw = strndup(str_value, str_len - strlen("%u128"));

                    if(! parse_unsigned_hex_word(raw, &value->u128)){
                        srsvm_const_free(value);
                        value = NULL;
                    }
                } else if(has_suffix(str_value, "%i128", true)){
                    value = srsvm_const_alloc(SRSVM_TYPE_I128);

                    raw = strndup(str_value, str_len - strlen("%i128"));

                    if(! parse_signed_hex_word(raw, &value->i128)){
                        srsvm_const_free(value);
                        value = NULL;
                    }
#endif
                } else {
                    value = srsvm_const_alloc(SRSVM_TYPE_WORD);

                    raw = srsvm_strdup(str_value);

                    if(! parse_unsigned_hex_word(raw, &value->word)){
                        dbg_puts("word parse failed");
                        srsvm_const_free(value);
                        value = NULL;
                    } else {
                        dbg_printf("parsed constant as word: " PRINT_WORD_HEX, PRINTF_WORD_PARAM(value->word));
                    }
                }
            } else {
                value = srsvm_const_alloc(SRSVM_TYPE_WORD);

                raw = srsvm_strdup(str_value);

                if(! parse_unsigned_hex_word(raw, &value->word)){
                    dbg_puts("word parse failed");
                    srsvm_const_free(value);
                    value = NULL;
                } else {
                    dbg_printf("parsed constant as word: " PRINT_WORD_HEX, PRINTF_WORD_PARAM(value->word));
                }
            }
        }

        if(raw != NULL){
            free(raw);
        }
    }

    return value;
}


static srsvm_opcode* parse_opcode(const srsvm_opcode_map *opcode_map, const char* opcode_str)
{
    srsvm_opcode *opcode = NULL;

    if(opcode_map == NULL){
        dbg_puts("map null");
    } else if(opcode_str == NULL){
        dbg_puts("str null");
    }

    if(opcode_map != NULL && opcode_str != NULL && strlen(opcode_str) > 0)
    {
        srsvm_word opcode_code;

        if((opcode = opcode_lookup_by_name(opcode_map, opcode_str)) == NULL){

            if(! parse_unsigned_hex_word(opcode_str, &opcode_code)){

            } else {
                opcode = opcode_lookup_by_code(opcode_map, opcode_code);
            }
        }
    }

    return opcode;
}

typedef struct
{
    bool is_loaded;
    srsvm_word slot_num;
} asm_mod_tag;

bool srsvm_asm_line_parse(srsvm_assembly_program *program, const char* line_str, const char* input_filename, const unsigned long line_number)
{
    static char error_message_buf[4096] = { 0 };
#define ERR(msg) do { report_error(program, input_filename, line_number, msg, program->io_config); goto error_cleanup; } while(0)
#define ERR_fmt(fmt, ...) do { snprintf(error_message_buf, sizeof(error_message_buf), fmt, __VA_ARGS__); ERR(error_message_buf); } while(0)

#define WARN(msg) do { report_error(program, input_filename, line_number, msg, program->io_config); } while(0)
#define WARN_fmt(fmt, ...) do { snprintf(error_message_buf, sizeof(error_message_buf), fmt, __VA_ARGS__); WARN(error_message_buf); } while(0)

    bool success = false;

    srsvm_assembly_line *line = srsvm_asm_line_alloc();

    if(line == NULL){
        //goto error_cleanup;
    }

    line->line_number = line_number;

#if defined(DEBUG)
    strncpy(line->original_line, line_str, sizeof(line->original_line));
#endif

    dbg_printf("input line: '%s'",  line_str);

    size_t line_len = strlen(line_str);
    char *writable_line = srsvm_strdup(line_str);

    if(writable_line == NULL){
        goto error_cleanup;
    }

    char* label = NULL;
    char* module = NULL;
    char* opcode = NULL;
    char* args[MAX_INSTRUCTION_ARGS] = { NULL };
    int argc = 0;

    bool in_arg_str = false; 
    bool in_arg_esc = false;

    line_lexer_state lex_state = LEX_INIT;

    dbg_printf("lexing '%s'", writable_line);

    for(size_t i = 0; i < line_len && lex_state != LEX_ERROR && lex_state != LEX_COMMENT; i++){
        char c = writable_line[i];

        if(c == 0) break;

        switch(lex_state){
            case LEX_INIT:
                if(! isspace(c)){
                    opcode = writable_line + i;
                    lex_state = LEX_OPCODE;
                }

                break;

            case LEX_LABEL:
                if(label == NULL){
                    label = writable_line + i;
                }

                if(isspace(c)){
                    lex_state = LEX_OPCODE;
                } else if(!isalpha(c) && !isdigit(c)){
                    switch(c){
                        case ':':
                            writable_line[i] = 0;
                            lex_state = LEX_OPCODE;
                            break;
                    }
                }
                break;

            case LEX_OPCODE:
                if(opcode == NULL){
                    opcode = writable_line + i;
                }

                if(isspace(c)){
                    if(opcode != NULL){
                        writable_line[i] = 0;
                        lex_state = LEX_ARG;
                    }
                } else if(!isalpha(c) && !isdigit(c) && c != '_'){
                    switch(c){
                        case ':':
                            if(opcode != NULL){
                                writable_line[i] = 0;
                                label = opcode;
                                opcode = NULL;
                                lex_state = LEX_LABEL;
                            } else {
                                lex_state = LEX_ERROR;
                            }
                            break;

                        case '.':
                            break;

                        case ';':
                            writable_line[i] = 0;
                            lex_state = LEX_COMMENT;
                            break;

                        default:
                            lex_state = LEX_ERROR;
                            break;
                    }
                }
                break;

            case LEX_ARG:
                if(argc >= MAX_INSTRUCTION_ARGS){
                    dbg_printf("bailing on too many args: %d\n", argc);
                    argc = MAX_INSTRUCTION_ARGS;
                    lex_state = LEX_ERROR;
                } else {
                    if(args[argc] == NULL){
                        if(! isspace(c) && c != ';'){
                            dbg_printf("found arg at char '%c'", c);
                            args[argc] = writable_line + i;
                        }
                    } else {
                        if((isspace(c) || c == ';') && !in_arg_esc && !in_arg_str){
                            writable_line[i] = 0;
                            if(args[argc] != NULL){
                                dbg_printf("ending arg at char '%c'", c);
                                argc++;
                            }
                        }
                    }

                    if(in_arg_str){
                        if(in_arg_esc){
                            in_arg_esc = false;
                        } else {
                            switch(c){
                                case '\\':
                                    in_arg_esc = true;
                                    break;

                                case '"':
                                    in_arg_str = false;
                                    if(args[argc] != NULL){
                                        dbg_printf("ending arg at char '%c'", c);
                                        argc++;
                                    }
                                    break;
                            }
                        }
                    } else {
                        if(! in_arg_esc){
                            if(isspace(c)){
                                writable_line[i] = 0;
                                if(args[argc] != NULL){
                                    dbg_printf("ending arg at char '%c'", c);
                                    argc++;
                                }
                            } else {
                                switch(c){
                                    case '"':
                                        in_arg_str = true;
                                        break;

                                    case ';':
                                        writable_line[i] = 0;
                                        if(args[argc] != NULL){
                                            dbg_printf("ending arg at char '%c'", c);
                                            argc++;
                                        }

                                        lex_state = LEX_COMMENT;
                                        break;

                                    default:
                                        break;
                                }
                            }
                        } else {
                            in_arg_esc = false;
                        }
                    }
                }
                break;

            default:
                break;
        }
    }

    if(in_arg_str){ // error: unterminated string

        dbg_puts("unterminated string");
        goto error_cleanup;
    }

    if(lex_state == LEX_ERROR){
        dbg_puts("lexing failed");
        goto error_cleanup;
    }

    if(lex_state == LEX_ARG){
        if(args[argc] != NULL){
            argc++;
        }
    }

    success = true;

    bool insert_label = false;

    if(label != NULL){
        strncpy(line->label, label, sizeof(line->label));

        if(srsvm_string_map_contains(program->label_map, label)){
            goto error_cleanup;
        }

        insert_label = true;
    } else {
        line->label[0] = 0;
    }

    int mod_op_offset = 0;

    if(opcode != NULL && strlen(opcode) > 0){

        dbg_printf("opcode = '%s'", opcode);

        char *mod_separator = strrchr(opcode, '.');

        if(mod_separator != NULL){
            module = opcode;
            *mod_separator = 0;
            opcode = mod_separator + 1;

            strncpy(line->module_name, module, sizeof(line->module_name));

            if(*opcode == 0){
                goto error_cleanup;
            }

            srsvm_module *mod = NULL;

            if(! srsvm_string_map_contains(program->mod_map, module)){
                bool search_multilib = true;
                char *mod_filename = NULL;

search_again:
                mod_filename = srsvm_module_find(module, NULL, program->module_search_path, search_multilib);

                if(mod_filename == NULL){
                    ERR_fmt("failed to locate module '%s'", module);
                } else {
                    mod = srsvm_module_alloc(module, mod_filename, program->mod_map->count);

                    if(mod == NULL){
                        ERR_fmt("failed to load module file '%s'", mod_filename);
                    }

                    if(mod != NULL){
                        if((mod->tag = malloc(sizeof(asm_mod_tag))) == NULL){
                            ERR_fmt("failed to allocate module tag: %s", strerror(errno));
                        } else {
                            ((asm_mod_tag*) mod->tag)->is_loaded = false;
                        }
                        
                        mod->ref_count = 1;

                        if(! srsvm_string_map_insert(program->mod_map, module, mod)){
                            srsvm_module_free(mod);
                            mod = NULL;

                            goto error_cleanup;
                        }
                    } else if(search_multilib && (mod_filename == NULL || mod == NULL)){
                        search_multilib = false;

                        goto search_again;
                    }
                }

                if(mod == NULL){
                    ERR_fmt("failed to load module '%s'", module);
                }
            } else {
                mod = srsvm_string_map_lookup(program->mod_map, module);

                if(mod == NULL){
                    ERR_fmt("failed to locate module '%s'", module);
                }

                mod->ref_count++;
            }

            line->opcode = parse_opcode(mod->opcode_map, opcode);

            //line->opcode = parse_opcode(mod->opcode_map, opcode);

            mod_op_offset = 2;
        } else {
            line->module_name[0] = 0;

            line->opcode = parse_opcode(program->opcode_map, opcode); 
        }

        if(line->opcode == NULL){
            ERR_fmt("failed to resolve opcode '%s'", opcode);
        }

    } else {
        line->opcode = program->builtin_NOP;
    }

    line->argc = argc + mod_op_offset;

    dbg_printf("line argc = " PRINT_WORD, PRINTF_WORD_PARAM(line->argc));

    if(argc < line->opcode->argc_min){
        WARN_fmt("line has %lu args, outside expected range [%u, %u]", (unsigned long) argc, line->opcode->argc_min, line->opcode->argc_max);
    } else if(argc > line->opcode->argc_max){
        WARN_fmt("line has %lu args, outside expected range [%u, %u]", (unsigned long) argc, line->opcode->argc_min, line->opcode->argc_max);
    }

    for(int i = 0; i < argc; i++){
        char *arg = args[i];

        dbg_printf("args[%i] = '%s'", i, arg);

        char *const_str = arg;
        srsvm_assembly_constant *const_val;
        srsvm_constant_value *parsed;

        switch(arg[0]){
            case '$':
                if(strlen(arg) == 1){
                    ERR("truncated register name");
                } 

                srsvm_assembly_register *reg;
                if(srsvm_string_map_contains(program->reg_map, arg)){
                    reg = srsvm_string_map_lookup(program->reg_map, arg);

                    reg->ref_count++;
                } else {
                    reg = malloc(sizeof(srsvm_assembly_register));

                    if(reg == NULL){
                        ERR_fmt("failed to allocate register: %s", strerror(errno));
                    }

                    strncpy(reg->name, arg, sizeof(reg->name));
                    reg->ref_count = 1;

                    if(! srsvm_string_map_insert(program->reg_map, reg->name, reg)){
                        free(reg);
                        ERR_fmt("failed to map register '%s'", reg->name);
                    }
                }

                line->register_references[line->num_register_refs].reg = reg;
                line->register_references[line->num_register_refs].arg_index = i + mod_op_offset;
                line->num_register_refs++;

                break;

            case '#':
                if(strlen(arg) == 1){
                    ERR("truncated label name");
                }

                strncpy(line->jump_target, arg+1, sizeof(line->jump_target));

                break;

            default:
                parsed = parse_const(const_str); 

                if(parsed != NULL){

                    if(srsvm_string_map_contains(program->const_map, const_str)){
                        const_val = srsvm_string_map_lookup(program->const_map, const_str);
                        const_val->ref_count++;

                        srsvm_const_free(parsed);
                        parsed = const_val->value;
                    } else {
                        const_val = malloc(sizeof(srsvm_assembly_constant));

                        if(const_val == NULL){
                            ERR_fmt("failed to allocate constant: %s", strerror(errno));
                        }

                        const_val->value = parsed;
                        const_val->ref_count = 1;

                        if(! srsvm_string_map_insert(program->const_map, const_str, const_val)){
                            srsvm_const_free(parsed);
                            free(const_val);

                            ERR_fmt("failed to map constant '%s'", const_str);
                        }
                    }

                    if(parsed->type == SRSVM_TYPE_WORD && line->opcode != program->builtin_LOAD_CONST){
                        line->assembled_instruction.argv[i + mod_op_offset] = parsed->word;
                    } else {
                        line->constant_references[line->num_constant_refs].c = const_val;
                        line->constant_references[line->num_constant_refs].arg_index = i + mod_op_offset;
                        line->num_constant_refs++;
                    }
                }
                break;
        }
    }

    if(insert_label){
        srsvm_string_map_insert(program->label_map, line->label, line);
    }

    if(program->lines == NULL){
        program->lines = line;
        line->prev = NULL;
    } else {
        line->prev = program->last_line;
        program->last_line->next = line;
    }
    program->last_line = line;
    line->next = NULL;
    program->line_count++;

    return success;

error_cleanup:
    if(writable_line != NULL){
        //free(writable_line);
    }

    if(line != NULL){
        srsvm_asm_line_free(line);
    }

    return false;
#undef ERR
#undef ERR_fmt

#undef WARN
#undef WARN_fmt
}

void srsvm_asm_program_set_search_path(srsvm_assembly_program *program, const char** search_path)
{
    if(program != NULL){
        if(search_path != NULL){
            size_t path_count = 0;
            const char* path;
            do {
                path = search_path[path_count++];
            } while(path != NULL);

            char ** arr = malloc(path_count * sizeof(char*));
            memset(arr, 0, (path_count * sizeof(char*)));
            for(size_t i = 0; i < path_count - 1; i++){
                arr[i] = srsvm_strdup(search_path[i]);
            }

            program->module_search_path = arr;
        } else {
			const char** path = malloc(sizeof(char*) * 2);
			path[0] = "mod";
			path[1] = NULL;

			srsvm_asm_program_set_search_path(program, path);

			free(path);
        }
    }
}

typedef struct
{
    void **data;
    size_t count;
} flattened_tree;

static void assign_next_value(const char* key, void* value, void* arg)
{
    flattened_tree *flat_data = (flattened_tree*) arg;

    flat_data->data[flat_data->count++] = value;
}

static int compare_registers(const void* a, const void* b)
{
    srsvm_assembly_register *reg_a = *(srsvm_assembly_register**) a;
    srsvm_assembly_register *reg_b = *(srsvm_assembly_register**) b;

    return reg_b->ref_count - reg_a->ref_count;
}

static bool flatten_registers(srsvm_assembly_program *program)
{
    bool success = false;

    if(program->registers != NULL){
        free(program->registers);
    }

    program->registers = malloc(sizeof(srsvm_assembly_register*) * program->reg_map->count);

    program->num_registers = 0;

    if(program->registers != NULL){

        flattened_tree flat_data;
        flat_data.data = (void**)program->registers;
        flat_data.count = 0;

        srsvm_string_map_walk(program->reg_map, assign_next_value, &flat_data);

        qsort(flat_data.data, flat_data.count, sizeof(srsvm_assembly_register*), compare_registers);

        dbg_printf("found %ld registers", flat_data.count);

        for(srsvm_word i = 0; i < flat_data.count; i++){
            program->registers[i]->slot_num = i;
        }

        program->num_registers = flat_data.count;

        success = true;
    }

    return success;
}

static int compare_constants(const void* a, const void* b)
{
    srsvm_assembly_constant *const_a = *(srsvm_assembly_constant**) a;
    srsvm_assembly_constant *const_b = *(srsvm_assembly_constant**) b;

    return const_b->ref_count - const_a->ref_count;
}

static bool flatten_constants(srsvm_assembly_program *program)
{
    bool success = false;

    if(program->constants != NULL){
        free(program->constants);
    }

    program->constants = malloc(sizeof(srsvm_assembly_constant*) * program->const_map->count);

    program->num_constants = 0;

    if(program->constants != NULL){

        flattened_tree flat_data;
        flat_data.data = (void**) program->constants;
        flat_data.count = 0;

        srsvm_string_map_walk(program->const_map, assign_next_value, &flat_data);

        qsort(flat_data.data, flat_data.count, sizeof(srsvm_assembly_constant*), compare_constants);

        dbg_printf("found %ld constants", flat_data.count);

        for(srsvm_word i = 0; i < flat_data.count; i++){
            program->constants[i]->slot_num = i;
        }

        program->num_constants = flat_data.count;

        success = true;
    }

    return success;
}

static long compute_byte_offset(srsvm_assembly_line *from, srsvm_assembly_line *to)
{
    long bytes = 0;

    if(from->line_number < to->line_number){
        for(srsvm_assembly_line *line = from; line != to; line = line->next){
            bytes += line->assembled_size;
        }
    } else if(from->line_number > to->line_number){
        for(srsvm_assembly_line *line = to; line != from; line = line->next){
            bytes -= line->assembled_size;
        }

        bytes += to->assembled_size;
        bytes -= from->assembled_size;
    }

    return bytes;
}

typedef struct 
{
    srsvm_assembly_line **line_ptr;
    srsvm_opcode *unload_opcode;
    srsvm_word next_slot;
} mod_cache_data;

static void module_lru_evict(const char* mod_name, void* evicted_data, void* state)
{
    srsvm_module *mod = evicted_data;

    mod_cache_data *state_data = state;

    asm_mod_tag *tag = mod->tag;

    srsvm_assembly_line *line = *state_data->line_ptr;

    if(line->pre_count >= SRSVM_ASM_MAX_PREFIX_SUFFIX_INSTRUCTIONS){
        // TODO: error
    } else {
        line->pre[line->pre_count].opcode = state_data->unload_opcode->code;
        line->pre[line->pre_count].argv[0] = tag->slot_num;
        line->pre[line->pre_count].argc = 1;
        line->pre_count++;

        state_data->next_slot = tag->slot_num;

        tag->is_loaded = false;
    }
}

static void unload_modules(const char* key, void *data, void* arg)
{
    srsvm_module *mod = data;

    asm_mod_tag *tag = mod->tag;

    tag->is_loaded = false;
}

srsvm_program *srsvm_asm_emit(srsvm_assembly_program *program, const srsvm_ptr entry_point, const srsvm_word word_alignment)
{ 
    srsvm_program *out_program = srsvm_program_alloc();

    if(program != NULL && out_program != NULL){

        static char error_message_buf[4096] = { 0 };

#define ERR(msg) do { report_error(program, "<output>", 0, msg, program->io_config); goto error_cleanup; } while(0)
#define ERR_fmt(fmt, ...) do { snprintf(error_message_buf, sizeof(error_message_buf), fmt, __VA_ARGS__); ERR(error_message_buf); } while(0)

#define WARN(msg) do { report_error(program, "<output>", 0, msg, program->io_config); } while(0)
#define WARN_fmt(fmt, ...) do { snprintf(error_message_buf, sizeof(error_message_buf), fmt, __VA_ARGS__); WARN(error_message_buf); } while(0)

        srsvm_program_metadata *metadata = srsvm_program_metadata_alloc();
        metadata->word_size = WORD_SIZE;
        metadata->entry_point = entry_point;
        out_program->metadata = metadata;

        srsvm_assembly_register *module_register = malloc(sizeof(srsvm_assembly_register));

        if(module_register == NULL){
            ERR_fmt("failed to allocate built-in $$MOD register: %s", strerror(errno));
        } else {
            strncpy(module_register->name, "$$MOD", sizeof(module_register->name));
            if(! srsvm_string_map_insert(program->reg_map, module_register->name, module_register)){
                ERR_fmt("failed to map built-in $$MOD register: %s", strerror(errno));
            }
            program->num_registers = 1;
        }

        srsvm_register_specification *assembled_mod_reg = NULL;

        srsvm_assembly_line *line = NULL;

        mod_cache_data state_data;
        state_data.next_slot = 0;
        state_data.unload_opcode = program->builtin_CMOD_UNLOAD;
        state_data.line_ptr = &line;

        srsvm_lru_cache *mod_cache = srsvm_lru_cache_alloc(true, SRSVM_MODULE_MAX_COUNT / 4, module_lru_evict, &state_data);

        if(mod_cache == NULL){
            ERR_fmt("failed to allocate LRU module cache: %s", strerror(errno));
        }

        program->assembled_size = 0;

        for(line = program->lines; line != NULL; line = line->next){
            if(strlen(line->module_name) > 0){
                char mod_name_literal[SRSVM_MODULE_MAX_NAME_LEN+2] = { 0 };
                snprintf(mod_name_literal, sizeof(mod_name_literal), "\"%s\"", line->module_name);

                srsvm_assembly_constant *const_val;

                if(srsvm_string_map_contains(program->const_map, mod_name_literal)){
                    const_val = srsvm_string_map_lookup(program->const_map, mod_name_literal);
                    const_val->ref_count++;
                } else {
                    const_val = malloc(sizeof(srsvm_assembly_constant));

                    if(const_val == NULL){
                        ERR_fmt("failed to allocate constant value: %s", strerror(errno));
                    }

                    const_val->value = srsvm_const_alloc(SRSVM_TYPE_STR);
                    const_val->value->str = srsvm_strdup(line->module_name);
                    const_val->value->str_len = strlen(line->module_name);
                    const_val->ref_count = 1;

                    if(! srsvm_string_map_insert(program->const_map, mod_name_literal, const_val)){
                        ERR_fmt("failed to map constant value: '%s'", mod_name_literal);
                    }
                }
            }
        }

        if(! flatten_registers(program)){
            ERR("failed to flatten program registers");
        } else {
            srsvm_register_specification *reg = out_program->registers;
            for(int r = 1; r < out_program->num_registers; r++){
                reg = reg->next;
            }

            srsvm_assembly_register *asm_reg;
            for(int r = 0; r < program->num_registers; r++){
                asm_reg = program->registers[r];

                srsvm_register_specification *new_reg = srsvm_program_register_alloc();
                dbg_printf("made reg " PRINT_WORD_HEX ":%s", PRINTF_WORD_PARAM(asm_reg->slot_num), asm_reg->name);
                strncpy(new_reg->name, asm_reg->name, sizeof(new_reg->name));
                new_reg->name_len = strlen(asm_reg->name);
                new_reg->index = asm_reg->slot_num;
                new_reg->next = NULL;

                if(asm_reg == module_register){
                    assembled_mod_reg = new_reg;
                }

                if(reg == NULL){
                    out_program->registers = new_reg;
                } else {
                    reg->next = new_reg;
                }

                reg = new_reg;

                out_program->num_registers++;
            }
        }

        if(! flatten_constants(program)){
            ERR("failed to flatten program constants");
        } else {
            srsvm_constant_specification *c_out =  out_program->constants;

            for(int c = 1; c < out_program->num_constants; c++){
                c_out = c_out->next;
            }

            srsvm_assembly_constant *asm_const;
            for(int c = 0; c < program->num_constants; c++){
                asm_const = program->constants[c];

                srsvm_constant_specification *new_const = srsvm_program_const_alloc();
                new_const->const_val = *asm_const->value;
                new_const->const_slot = asm_const->slot_num;

                new_const->next = NULL;

                if(c_out == NULL){
                    out_program->constants = new_const;
                } else {
                    c_out->next = new_const;
                }

                c_out = new_const;

                out_program->num_constants++;
            }
        }

#if defined(SRSVM_SUPPORT_COMPRESSION)
        out_program->constants_compressed = true;
#else
        out_program->constants_compressed = false;
#endif

        for(line = program->lines; line != NULL; line = line->next){
            line->assembled_ptr = entry_point + program->assembled_size;

            srsvm_opcode *line_op = line->opcode;

            if(line_op == program->builtin_MOD_UNLOAD_ALL){
                srsvm_string_map_walk(mod_cache->map, unload_modules, NULL);
                srsvm_lru_cache_clear(mod_cache, false);
                state_data.next_slot = 0;
            }

            if(strlen(line->module_name) > 0){
                char mod_name_literal[SRSVM_MODULE_MAX_NAME_LEN+2] = { 0 };
                snprintf(mod_name_literal, sizeof(mod_name_literal), "\"%s\"", line->module_name);

                srsvm_assembly_constant *mod_name_const = srsvm_string_map_lookup(program->const_map, mod_name_literal);

                srsvm_module *mod = srsvm_lru_cache_lookup(mod_cache, line->module_name);
                asm_mod_tag *tag;

                if(mod == NULL){
                    mod = srsvm_string_map_lookup(program->mod_map, line->module_name);

                    if(mod == NULL){
                        ERR_fmt("failed to locate module '%s'", line->module_name);
                    } else {
                        tag = mod->tag;

                        if(tag->is_loaded){
                            WARN_fmt("cache miss for loaded module '%s'", line->module_name);
                        } else if(! srsvm_lru_cache_insert(mod_cache, mod->name, mod)){
                            ERR_fmt("cache insert failed for module '%s'", line->module_name);
                        } else {
                            tag->slot_num = state_data.next_slot++;
                            //tag->is_loaded = true;
                        }
                    }
                } else {
                    tag = mod->tag;
                }

                if(! tag->is_loaded){
                    if(line->pre_count + 2 > SRSVM_ASM_MAX_PREFIX_SUFFIX_INSTRUCTIONS){
                        ERR_fmt("not enough space to insert module load '%s'", line->module_name);
                    } else {
                        line->pre[line->pre_count].opcode = program->builtin_LOAD_CONST->code;
                        line->pre[line->pre_count].argv[0] = assembled_mod_reg->index;
                        line->pre[line->pre_count].argv[1] = mod_name_const->slot_num;
                        line->pre[line->pre_count].argc = 2;

                        line->pre_count++;

                        line->pre[line->pre_count].opcode = program->builtin_CMOD_LOAD->code;
                        line->pre[line->pre_count].argv[0] = assembled_mod_reg->index;
                        line->pre[line->pre_count].argv[1] = tag->slot_num;
                        line->pre[line->pre_count].argv[2] = assembled_mod_reg->index;
                        line->pre[line->pre_count].argc = 3;

                        line->pre_count++;

                        tag->is_loaded = true;
                    }
                }

                line->assembled_instruction.opcode = program->builtin_CMOD_OP->code;
                line->assembled_instruction.argv[0] = tag->slot_num;
                line->assembled_instruction.argv[1] = line_op->code;
                dbg_printf("mod op code = " PRINT_WORD_HEX, PRINTF_WORD_PARAM(line_op->code));
            } else {
                line->assembled_instruction.opcode = line_op->code;
            }

            for(unsigned i = 0; i < line->num_register_refs; i++){
                line->assembled_instruction.argv[line->register_references[i].arg_index] = line->register_references[i].reg->slot_num;
            }

            for(unsigned i = 0; i < line->num_constant_refs; i++){
                line->assembled_instruction.argv[line->constant_references[i].arg_index] = line->constant_references[i].c->slot_num;
            }
            
            //line->assembled_instruction.opcode = line_op->code;
            line->assembled_instruction.argc = line->argc;

            line->assembled_size = 0;

            for(int i = 0; i < line->pre_count; i++){
                size_t instruction_size = (1 + line->pre[i].argc) * sizeof(srsvm_word);

                if(word_alignment > 0){
                    instruction_size += ((word_alignment - (instruction_size % word_alignment)) % word_alignment) * sizeof(srsvm_word);
                }

                line->assembled_size += instruction_size;
            }

            line->assembled_size += (1 + line->argc) * sizeof(srsvm_word);

            if(word_alignment > 0){
                line->assembled_size += ((word_alignment - (((1 + line->argc) * sizeof(srsvm_word)) % word_alignment)) % word_alignment) * sizeof(srsvm_word);
            }

            for(int i = 0; i < line->post_count; i++){
                size_t instruction_size = (1 + line->post[i].argc) * sizeof(srsvm_word);

                if(word_alignment > 0){
                    instruction_size += ((word_alignment - (instruction_size % word_alignment)) % word_alignment) * sizeof(srsvm_word);
                }

                line->assembled_size += instruction_size;
            }

            program->assembled_size += line->assembled_size;
        }

        for(line = program->lines; line != NULL; line = line->next){
            if(strlen(line->jump_target) > 0){
                srsvm_assembly_line *target_line = srsvm_string_map_lookup(program->label_map, line->jump_target);

                if(target_line == NULL){
                    ERR_fmt("failed to locate label '%s'", line->jump_target);
                } else {
                    long offset = compute_byte_offset(line, target_line);

                    if(offset < 0){
                        if(-offset > SRSVM_MAX_PTR){
                            ERR_fmt("jump distance (%lu bytes) too large to fit in a WORD value", offset);
                        } else {

                            if(line->opcode == program->builtin_JMP){
                                line->assembled_instruction.opcode = program->builtin_CJMP_BACK->code;
                            } else if(line->opcode == program->builtin_JMP_IF){
                                line->assembled_instruction.opcode = program->builtin_CJMP_BACK_IF->code;
                            } else if(line->opcode == program->builtin_JMP_ERR){
                                line->assembled_instruction.opcode = program->builtin_CJMP_BACK_ERR->code;
                            } else {
                                ERR_fmt("failed to map jump for target label '%s'", line->jump_target);
                            }

                            line->assembled_instruction.argv[line->jump_target_arg_index] = (srsvm_word) -offset;
                        }
                    } else {
                        if(offset > SRSVM_MAX_PTR){
                            ERR_fmt("jump distance (%lu bytes) too large to fit in a WORD value", offset);
                        } else {
                            if(line->opcode == program->builtin_JMP){
                                line->assembled_instruction.opcode = program->builtin_CJMP_FORWARD->code;
                            } else if(line->opcode == program->builtin_JMP_IF){
                                line->assembled_instruction.opcode = program->builtin_CJMP_FORWARD_IF->code;
                            } else if(line->opcode == program->builtin_JMP_ERR){
                                line->assembled_instruction.opcode = program->builtin_CJMP_FORWARD_ERR->code;
                            } else {
                                ERR_fmt("failed to map jump for target label '%s'", line->jump_target);
                            }

                            line->assembled_instruction.argv[line->jump_target_arg_index] = (srsvm_word) offset;
                        }
                    }
                }
            }
        }

        if(entry_point + program->assembled_size > SRSVM_MAX_PTR){
            ERR("program data is too large to fit in VM memory");
        }

        srsvm_literal_memory_specification *program_memory = srsvm_program_lmem_alloc();
        out_program->literal_memory = program_memory;
        program_memory->start_address = entry_point;
#if defined(SRSVM_SUPPORT_COMPRESSION)
        program_memory->is_compressed = true;
#else
        program_memory->is_compressed = false;
#endif
        program_memory->size = program->assembled_size;
        if((program_memory->data = malloc(program->assembled_size)) == NULL){
            ERR_fmt("failed to allocate program memory: %s", strerror(errno));
        } else {
            srsvm_word *ptr = program_memory->data;

            size_t padding_nops;

            for(line = program->lines; line != NULL; line = line->next){
                for(int i = 0; i < line->pre_count; i++){
                    *(ptr++) = (OPCODE_MK_ARGC(line->pre[i].argc) | line->pre[i].opcode);
                    memcpy(ptr, &line->pre[i].argv, line->pre[i].argc * sizeof(srsvm_word));
                    ptr += line->pre[i].argc;

                    if(word_alignment > 0){
                        padding_nops = ((word_alignment - (((1 + line->pre[i].argc) * sizeof(srsvm_word)) % word_alignment)) % word_alignment) * sizeof(srsvm_word);

                        for(int j = 0; j < padding_nops; j++){
                            *(ptr++) = (OPCODE_MK_ARGC(0) | program->builtin_NOP->code);
                        }
                    }
                }

                *(ptr++) = (OPCODE_MK_ARGC(line->assembled_instruction.argc) | line->assembled_instruction.opcode);
                memcpy(ptr, &line->assembled_instruction.argv, line->assembled_instruction.argc * sizeof(srsvm_word));
                ptr += line->assembled_instruction.argc;
                
                if(word_alignment > 0){
                    padding_nops = ((word_alignment - (((1 + line->assembled_instruction.argc) * sizeof(srsvm_word)) % word_alignment)) % word_alignment) * sizeof(srsvm_word);
                    for(int j = 0; j < padding_nops; j++){
                        *(ptr++) = (OPCODE_MK_ARGC(0) | program->builtin_NOP->code);
                    }
                }

                for(int i = 0; i < line->post_count; i++){
                    *ptr = (OPCODE_MK_ARGC(line->post[i].argc) | line->post[i].opcode);
                    ptr++;
                    memcpy(ptr, &line->post[i].argv, line->post[i].argc * sizeof(srsvm_word));
                    ptr += line->pre[i].argc;

                    if(word_alignment > 0){
                        padding_nops = ((word_alignment - (((1 + line->post[i].argc) * sizeof(srsvm_word)) % word_alignment)) % word_alignment) * sizeof(srsvm_word);

                        for(int j = 0; j < padding_nops; j++){
                            *(ptr++) = (OPCODE_MK_ARGC(0) | program->builtin_NOP->code);
                        }
                    }
                }
            }
        }
        program_memory->readable = true;
        program_memory->executable = true;
        program_memory->locked = true;
        out_program->num_lmem_segments = 1;
    }

    return out_program;

error_cleanup:
    if(out_program != NULL){
        srsvm_program_free(out_program);
    }

    return NULL;

#undef ERR
#undef ERR_fmt

#undef WARN
#undef WARN_fmt
}
