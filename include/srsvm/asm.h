#pragma once

#include "srsvm/constant.h"
#include "srsvm/lru.h"
#include "srsvm/module.h"
#include "srsvm/opcode.h"
#include "srsvm/program.h"

#define SRSVM_ASM_LINE_MAX_LEN 2048

#define SRSVM_ASM_LABEL_MAX_LEN 256

typedef struct
{
    unsigned long ref_count;
    srsvm_constant_value *value;

    srsvm_word slot_num;
} srsvm_assembly_constant;

typedef struct
{
    char name[SRSVM_REGISTER_MAX_NAME_LEN];

    unsigned long ref_count;

    srsvm_word slot_num;
} srsvm_assembly_register;

typedef struct
{
    srsvm_assembly_register *reg;
    srsvm_word arg_index;
} srsvm_assembly_register_reference;

typedef struct
{
    srsvm_assembly_constant *c;
    srsvm_word arg_index;
} srsvm_assembly_constant_reference;

typedef struct srsvm_assembly_line srsvm_assembly_line;

#define SRSVM_ASM_MAX_PREFIX_SUFFIX_INSTRUCTIONS 3

struct srsvm_assembly_line
{
    unsigned long line_number;    

    srsvm_assembly_line *prev;
    srsvm_assembly_line *next;

    char label[SRSVM_ASM_LABEL_MAX_LEN];

    char jump_target[SRSVM_ASM_LABEL_MAX_LEN];
    srsvm_word jump_target_arg_index;

    char module_name[SRSVM_MODULE_MAX_NAME_LEN];

    unsigned num_register_refs;
    srsvm_assembly_register_reference register_references[MAX_INSTRUCTION_ARGS];

    unsigned num_constant_refs;
    srsvm_assembly_constant_reference constant_references[MAX_INSTRUCTION_ARGS];

    unsigned num_raw_values;
    srsvm_assembly_constant *raw_constants[MAX_INSTRUCTION_ARGS];

    srsvm_opcode* opcode;
    srsvm_word argc;

    srsvm_instruction pre[SRSVM_ASM_MAX_PREFIX_SUFFIX_INSTRUCTIONS];
    srsvm_word pre_count;
    
    srsvm_instruction post[SRSVM_ASM_MAX_PREFIX_SUFFIX_INSTRUCTIONS];
    srsvm_word post_count;

    srsvm_instruction assembled_instruction;
    size_t assembled_size;
    srsvm_ptr assembled_ptr;

#if DEBUG
    char original_line[SRSVM_ASM_LINE_MAX_LEN];
#endif
};

typedef void (srsvm_assembler_message_report_func)(const char*, const unsigned long, const char*, void*);

typedef struct
{
    unsigned long line_count;

    srsvm_word assembled_size;

    srsvm_assembly_line *lines;
    srsvm_assembly_line *last_line;

    srsvm_opcode_map *opcode_map;
    srsvm_string_map *label_map;
    srsvm_string_map *mod_map;
    srsvm_string_map *reg_map;
    srsvm_string_map *const_map;

    srsvm_assembly_register **registers;
    srsvm_word num_registers;
    srsvm_assembly_constant **constants;
    srsvm_word num_constants;

    char** module_search_path;

    char input_filename[SRSVM_MAX_PATH_LEN];

    srsvm_assembler_message_report_func *on_error;
    srsvm_assembler_message_report_func *on_warning;
    void* io_config;

    srsvm_opcode *builtin_LOAD_CONST;
    srsvm_opcode *builtin_MOD_LOAD;
    srsvm_opcode *builtin_MOD_UNLOAD;
    srsvm_opcode *builtin_MOD_UNLOAD_ALL;
    srsvm_opcode *builtin_MOD_OP;
    srsvm_opcode *builtin_NOP;
    srsvm_opcode *builtin_JMP;
    srsvm_opcode *builtin_JMP_IF;
    srsvm_opcode *builtin_JMP_ERR;
    srsvm_opcode *builtin_CJMP_FORWARD;
    srsvm_opcode *builtin_CJMP_FORWARD_IF;
    srsvm_opcode *builtin_CJMP_FORWARD_ERR;
    srsvm_opcode *builtin_CJMP_BACK;
    srsvm_opcode *builtin_CJMP_BACK_IF;
    srsvm_opcode *builtin_CJMP_BACK_ERR;


} srsvm_assembly_program;

srsvm_assembly_line *srsvm_asm_line_alloc(void);
void srsvm_asm_line_free(srsvm_assembly_line *line);


srsvm_assembly_program *srsvm_asm_program_alloc(srsvm_assembler_message_report_func on_error, srsvm_assembler_message_report_func on_warning, void* io_config);
void srsvm_asm_program_free(srsvm_assembly_program *program);

void srsvm_asm_program_set_search_path(srsvm_assembly_program *program, const char** search_path);

bool srsvm_asm_line_parse(srsvm_assembly_program *program, const char* line_str, const char* input_filename, unsigned long line_number);

srsvm_program *srsvm_asm_emit(srsvm_assembly_program *program, const srsvm_ptr entry_point, const srsvm_word word_alignment);
