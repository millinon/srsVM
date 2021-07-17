#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "srsvm/asm.h"
#include "srsvm/debug.h"
#include "srsvm/opcode.h"
#include "srsvm/program.h"

#define STR(x) #x
#define STR_HELPER(x) STR(x)

#define PROG_NAME "srsvm_as_" STR_HELPER(WORD_SIZE)

void write_error(const char* message)
{
        fprintf(stderr, "Error: %s\n", message);
}

void show_usage(const char* error)
{
    if(error != NULL){
        write_error(error);
    }

    fprintf(stderr, "Usage: %s [optional args] program_name.s\n", PROG_NAME);
    fprintf(stderr, "\n");
    fprintf(stderr, "    optional arguments:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "      -ws <WORD_SIZE>       : specify target output word size (16/32/64/128)\n");
    fprintf(stderr, "      -o output_file.svm    : specify output filename\n");
    fprintf(stderr, "      -A <alignment>        : specify target output alignment (default: 0)\n");
    fprintf(stderr, "      -L <mod_search_path>  : specify module search path (semicolon separated)\n");
    fprintf(stderr, "      -D  |  --debug        : enable srsvm debug messages\n");
    fprintf(stderr, "      -h  |  --help         : show this help information\n");

    if(error != NULL){
        exit(1);
    } else exit(0);
}

bool srsvm_debug_mode;

typedef struct
{
    FILE *err_stream;
    FILE *warn_stream;

    bool warnings_suppressed;
    bool warnings_fatal;

    bool warning_issued;
} srsvm_as_io_config;

static inline void report_error(const char* filename, const unsigned long line_number, const char* message, void* config)
{
    srsvm_as_io_config *io_config = config;

    if(io_config != NULL){
        fprintf(io_config->err_stream, "Error: %s:%lu: %s\n", filename, line_number, message);
    }
}

static inline void report_warning(const char* filename, const unsigned long line_number, const char* message, void* config)
{
    srsvm_as_io_config *io_config = config;

    if(io_config != NULL){
        if(! io_config->warnings_suppressed){
            fprintf(io_config->warn_stream, "Warning: %s:%lu: %s\n", filename, line_number, message);
        }

        io_config->warning_issued = true;
    }
}

#define MAX_INPUT_FILES 16

#define MAX_INPUT_LINE_LEN 1024

int main(int argc, char *argv[]){
    srsvm_as_io_config io_config;
    io_config.warnings_suppressed = false;
    io_config.err_stream = stderr;
    io_config.warn_stream = stderr;
    io_config.warnings_fatal = false;
    io_config.warning_issued = false;

    srsvm_debug_mode = false;

    char *line_buf = malloc(MAX_INPUT_LINE_LEN * sizeof(char));

    size_t num_input_files = 0;
    char *input_files[MAX_INPUT_FILES] = { NULL };

    char* output_filename = NULL;

    char ** module_search_path = NULL;

    unsigned word_alignment = 0;
    bool alignment_specified = false;

    bool args_done = false;

    unsigned word_size = 0;

    for(int arg_i = 1; arg_i < argc; arg_i++){
        char* arg = argv[arg_i];

        if(arg[0] == '-' && !args_done){
            if(strcmp(arg, "-L") == 0){
                if(arg_i >= argc - 1){
                    show_usage("Error: -L specified with no arguments\n");
                } else if(module_search_path != NULL){
                    show_usage("Error: duplicate -L argument\n");
                } else {
                    char *lib_path = argv[++arg_i];
                    size_t path_len = strlen(lib_path);
                    size_t path_toks = 1;    

                    for(size_t i = 0; i < path_len; i++){
                        if(lib_path[i] == ';'){
                            path_toks++;
                        }
                    }

                    module_search_path = malloc((path_toks + 1) * sizeof(char*));

                    if(module_search_path == NULL){
                        fprintf(stderr, "Error: failed to allocate module search path\n");
                        return 1;
                    } else {
                        memset(module_search_path, 0, ((path_toks + 1) * sizeof(char*)));

                        size_t tok_num = 0;
                        char *tok = strtok(lib_path, ";");

                        if(tok == NULL){
                            if((module_search_path[0] = srsvm_strdup(lib_path)) == NULL){
                                fprintf(stderr, "Error: failed to allocate module search path token\n");
                                return 1;
                            }
                        } else {
                            while(tok != NULL){
                                if(strlen(tok) == 0){
                                    continue;
                                } else {
                                    if((module_search_path[tok_num++] = srsvm_strdup(tok)) == NULL){
                                        fprintf(stderr, "Error: failed to allocate module search path token\n");
                                    }
                                }

                                tok = strtok(NULL, ";");
                            }
                        }
                    }
                }
            } else if(strcmp(arg, "-D") == 0 || strcmp(arg, "--debug") == 0){
                if(srsvm_debug_mode){
                    show_usage("Error: duplicate -D argument\n");
                } else {
                    srsvm_debug_mode = true;
                }
            } else if(strcmp(arg, "-o") == 0){
                if(output_filename != NULL){
                    show_usage("Error: duplicate -o argument\n");
                    return 1;
                } else if(arg_i >= argc -1){
                    show_usage("Error: -o specified with no argument\n");
                    return 1;
                } else {
                    output_filename = argv[++arg_i];
                }
            } else if(strcmp(arg, "-A") == 0){
                if(alignment_specified){
                    show_usage("Error: duplicate -A argument\n");
                } else if(arg_i >= argc - 1){
                    show_usage("Error: -A specified with no argument\n");
                } else if(sscanf(argv[++arg_i], "%u", &word_alignment) != 1){
                    fprintf(stderr, "Error: failed to parse '%s' as unsigned integer\n", argv[arg_i]);
                    return 1;
                }
            } else if(strcmp(arg, "-ws") == 0){
                if(word_size != 0){
                    show_usage("Error: duplicate -ws argument\n");
                } else if(arg_i >= argc - 1){
                    show_usage("Error: -A specified with no argument\n");
                } else if(sscanf(argv[++arg_i], "%u", &word_size) != 1){
                    fprintf(stderr, "Error: failed to parse '%s' as unsigned integer\n", argv[arg_i]);
                    return 1;
                }
            } else if(strcmp(arg, "--") == 0){
                args_done = true;
            }
        } else {
            if(num_input_files >= MAX_INPUT_FILES){
                fprintf(stderr, "Error: too many input files specified: maximum is %u\n", MAX_INPUT_FILES);
                return 1;
            } else {
                input_files[num_input_files++] = arg;
            }
        }
    }

    if(word_size != 0 && word_size != WORD_SIZE){
        fprintf(stderr, "Error: word size mismatch: this executable only builds for WORD_SIZE = %u\n", WORD_SIZE);
        return 1;
    }

    if(num_input_files == 0){
        fprintf(stderr, "Error: no input files\n");
        return 1;
    }

    if(output_filename == NULL){
        if(strcmp(input_files[0], "-") == 0){
            output_filename = srsvm_strdup("out.svm");
            if(output_filename == NULL){
                fprintf(stderr, "Error: failed to allocate output filename\n");
                return 1;
            }
        } else {
            size_t output_buf_size = (strlen(input_files[0]) + 5) * sizeof(char);
            output_filename = malloc(output_buf_size);
            if(output_filename == NULL){
                fprintf(stderr, "Error: failed to allocate output filename\n");
                return 1;
            }
            strcpy(output_filename, input_files[0]);
            char *ext = strrchr(output_filename, '.');
            if(ext == NULL){
                strncat(output_filename, ".svm", strlen(output_filename) - strlen(input_files[0]) + 5);
            } else {
                strcpy(ext, ".svm");
            }
        }
    }

    srsvm_assembly_program *asm_prog = srsvm_asm_program_alloc(report_error, report_warning, &io_config);

    if(asm_prog == NULL){
        fprintf(stderr, "Error: failed to allocate output program\n");
        return 1;
    }


	srsvm_asm_program_set_search_path(asm_prog, (const char**) module_search_path);

    bool have_fatal_error = false;

    for(size_t input_num = 0; input_num < num_input_files; input_num++){
        FILE *input;
        const char* input_fname;

        dbg_printf("processing input file %s", input_files[input_num]);

        if(strcmp(input_files[input_num], "-") == 0){
            input = stdin;
            input_fname = "<stdin>";
        } else {
            input = fopen(input_files[input_num], "r");
            if(input == NULL){
                fprintf(stderr, "Error: failed to open input file '%s'\n", input_files[input_num]);
                return 1;
            }
            input_fname = input_files[input_num];
        }

        unsigned long line_num = 1;

        size_t size = MAX_INPUT_LINE_LEN;

		while(fgets(line_buf, size, input) != NULL){
        //while(getline(&line_buf, &size, input) != -1){
            line_buf[strcspn(line_buf, "\n")] = 0;
            line_buf[strcspn(line_buf, "\r")] = 0;

            if(! srsvm_asm_line_parse(asm_prog, line_buf, input_fname, line_num)){
                have_fatal_error = true;
                //break;
            } else if(io_config.warnings_fatal && io_config.warning_issued){
                have_fatal_error = true;
                //break;
            }

            line_num++;
        }

        if(input != stdin){
            fclose(input);
        }
    }

    /*srsvm_asm_line_parse(asm_prog, "LOAD_CONST $MSG \"Hello, world!\"", 1);
      srsvm_asm_line_parse(asm_prog, "PUTS $MSG", 2);
      srsvm_asm_line_parse(asm_prog, "HALT", 3);*/

    if(! have_fatal_error){

        srsvm_program *program = srsvm_asm_emit(asm_prog, 0x1000, (srsvm_word) word_alignment);

        if(! srsvm_program_serialize(output_filename, program)){
            fprintf(stderr, "Failed to serialize to %s\n", output_filename);
        } else {
            printf("Serialized to %s\n", output_filename);
        }
    
        srsvm_program_free(program);
    }

    srsvm_asm_program_free(asm_prog);
}
