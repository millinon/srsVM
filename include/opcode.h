#pragma once

#include <limits.h>
#include <stdbool.h>

#include "word.h"

#include "forward-decls.h"

#if WORD_SIZE == 16

#define MAX_INSTRUCTION_ARGS 3
#define OPCODE_ARGC_BITS 0b11000000
#define OPCODE_ARGC_BITS_SHIFT 6

#elif WORD_SIZE == 32

#define MAX_INSTRUCTION_ARGS 7
#define OPCODE_ARGC_BITS 0b11100000
#define OPCODE_ARGC_BITS_SHIFT 5

#elif WORD_SIZE == 64

#define MAX_INSTRUCTION_ARGS 15
#define OPCODE_ARGC_BITS 0b11110000
#define OPCODE_ARGC_BITS_SHIFT 4

#elif WORD_SIZE == 128

#define MAX_INSTRUCTION_ARGS 31
#define OPCODE_ARGC_BITS 0b11111000
#define OPCODE_ARGC_BITS_SHIFT 3

#endif

#define OPCODE_ARGC_MASK ((srsvm_word) OPCODE_ARGC_BITS << ((sizeof(srsvm_word) - 1) * CHAR_BIT))

#define OPCODE_ARGC(opcode) ((((opcode & OPCODE_ARGC_MASK) >> ((sizeof(srsvm_word) - 1) * CHAR_BIT)) & OPCODE_ARGC_BITS) >> OPCODE_ARGC_BITS_SHIFT)

#define OPCODE_MAX_NAME_LEN 255

typedef void (srsvm_opcode_func)(srsvm_vm*, srsvm_thread*, const srsvm_word argc, const srsvm_word argv[]);

typedef struct
{
    srsvm_word code;    
    char name[OPCODE_MAX_NAME_LEN];
    
    unsigned short argc_min;
    unsigned short argc_max;

    srsvm_opcode_func *func;
} srsvm_opcode;

typedef struct srsvm_opcode_map_node srsvm_opcode_map_node;

struct srsvm_opcode_map_node
{
    srsvm_opcode_map_node *name_parent;
    srsvm_opcode_map_node *code_parent;

    srsvm_opcode *opcode;

    srsvm_opcode_map_node *name_lchild;
    srsvm_opcode_map_node *name_rchild;
    
    srsvm_opcode_map_node *code_lchild;
    srsvm_opcode_map_node *code_rchild;
};

struct srsvm_opcode_map
{
    srsvm_lock lock;

    srsvm_opcode_map_node *by_name_root;
    srsvm_opcode_map_node *by_code_root;
    size_t count;
};

srsvm_opcode_map *srsvm_opcode_map_alloc(void);

bool opcode_name_exists(const srsvm_opcode_map* map, const char* opcode_name);
bool opcode_code_exists(const srsvm_opcode_map* map, const srsvm_word opcode_code);

srsvm_opcode *opcode_lookup_by_name(const srsvm_opcode_map* map, const char* opcode_name);
srsvm_opcode *opcode_lookup_by_code(const srsvm_opcode_map* map, const srsvm_word opcode_code);

bool opcode_map_insert(srsvm_opcode_map* map, srsvm_opcode* opcode);

typedef struct
{
    srsvm_word opcode;
    srsvm_word argv[MAX_INSTRUCTION_ARGS];
    srsvm_word argc;
} srsvm_instruction;

bool load_instruction(srsvm_vm *vm, const srsvm_ptr addr, srsvm_instruction *instruction);
