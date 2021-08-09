#pragma once

#include <limits.h>
#include <stdbool.h>

#include "srsvm/forward-decls.h"
#include "srsvm/impl.h"
#include "srsvm/map.h"
#include "srsvm/memory.h"
#include "srsvm/word.h"

#if WORD_SIZE == 16

#define OPCODE_ARGC_BITS 0b11100000
#define OPCODE_ARGC_BITS_SHIFT 5

#elif WORD_SIZE == 32

#define OPCODE_ARGC_BITS 0b11100000
#define OPCODE_ARGC_BITS_SHIFT 5

#elif WORD_SIZE == 64

#define OPCODE_ARGC_BITS 0b11110000
#define OPCODE_ARGC_BITS_SHIFT 4

#elif WORD_SIZE == 128

#define OPCODE_ARGC_BITS 0b11111000
#define OPCODE_ARGC_BITS_SHIFT 3

#endif

#define MAX_INSTRUCTION_ARGS ((unsigned)((srsvm_word)((OPCODE_ARGC_BITS) >> (OPCODE_ARGC_BITS_SHIFT))))

#define OPCODE_ARGC_MASK ((srsvm_word) OPCODE_ARGC_BITS << ((sizeof(srsvm_word) - 1) * CHAR_BIT))

#define OPCODE_ARGC(opcode) (((((opcode) & OPCODE_ARGC_MASK) >> ((sizeof(srsvm_word) - 1) * CHAR_BIT)) & OPCODE_ARGC_BITS) >> OPCODE_ARGC_BITS_SHIFT)

#define OPCODE_MK_ARGC(argc) ((((srsvm_word) argc) << OPCODE_ARGC_BITS_SHIFT) << ((sizeof(srsvm_word) - 1) * CHAR_BIT))

#define OPCODE_MAX_NAME_LEN 256


typedef uint8_t srsvm_arg_type;

#define SRSVM_ARG_TYPE_NONE 0
#define SRSVM_ARG_TYPE_WORD 1
#define SRSVM_ARG_TYPE_REGISTER 2
#define SRSVM_ARG_TYPE_CONSTANT 4

typedef struct
{
	srsvm_word value;

	srsvm_arg_type type;
} srsvm_arg;

typedef void (srsvm_opcode_func)(srsvm_vm*, srsvm_thread*, const srsvm_word argc, const srsvm_arg argv[]);

struct srsvm_opcode
{
    srsvm_word code;    
    char name[OPCODE_MAX_NAME_LEN];
    
    unsigned short argc_min;
    unsigned short argc_max;

    srsvm_opcode_func *func;
};

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
void srsvm_opcode_map_free(srsvm_opcode_map* map);

bool opcode_name_exists(const srsvm_opcode_map* map, const char* opcode_name);
bool opcode_code_exists(const srsvm_opcode_map* map, const srsvm_word opcode_code);

srsvm_opcode *opcode_lookup_by_name(const srsvm_opcode_map* map, const char* opcode_name);
srsvm_opcode *opcode_lookup_by_code(const srsvm_opcode_map* map, const srsvm_word opcode_code);

bool opcode_map_insert(srsvm_opcode_map* map, srsvm_opcode* opcode);

typedef struct
{
    srsvm_word opcode;
    
    srsvm_word argc;
    
    //srsvm_word argv[MAX_INSTRUCTION_ARGS];

    srsvm_arg argv[MAX_INSTRUCTION_ARGS];
} srsvm_instruction;

bool srsvm_opcode_load_instruction(srsvm_vm *vm, const srsvm_ptr addr, srsvm_instruction *instruction);

bool load_builtin_opcodes(srsvm_opcode_map *map);
