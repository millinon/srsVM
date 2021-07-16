#pragma once

#ifdef __unix__
#include <linux/limits.h>
#endif

#include <stdbool.h>
#include <stdint.h>

#if defined(WORD_SIZE)
#include "srsvm/constant.h" 
#include "srsvm/impl.h"
#include "srsvm/memory.h"
#include "srsvm/register.h"
#include "srsvm/word.h"
#endif

typedef struct
{
#if defined(SRSVM_PROGRAM_SUPPORT_SHEBANG)
    char shebang[SRSVM_MAX_PATH_LEN];
#endif
    char magic[3];
    uint8_t word_size;

#if defined(WORD_SIZE)
    srsvm_ptr entry_point;
#endif

} srsvm_program_metadata;

#if defined(WORD_SIZE)
typedef struct srsvm_register_specification srsvm_register_specification;

struct srsvm_register_specification
{
    uint16_t name_len;
    char name[SRSVM_REGISTER_MAX_NAME_LEN];
    uint32_t index;    
    
    srsvm_register_specification *next;
};

typedef struct srsvm_virtual_memory_specification srsvm_virtual_memory_specification;

struct srsvm_virtual_memory_specification
{
    srsvm_ptr start_address;
    srsvm_word size;

    srsvm_virtual_memory_specification *next;
};

typedef struct srsvm_literal_memory_specification srsvm_literal_memory_specification;

struct srsvm_literal_memory_specification
{
    srsvm_ptr start_address;
    srsvm_word size;

    bool is_compressed;
    size_t compressed_size;

    bool readable;
    bool writable;
    bool executable;

    bool locked;

    void *data;
    
    srsvm_literal_memory_specification *next;
};

typedef struct srsvm_constant_specification srsvm_constant_specification;

struct srsvm_constant_specification
{
    srsvm_word const_slot;

    srsvm_constant_value const_val;
    
    srsvm_constant_specification *next;
};

#endif

typedef struct
{
    srsvm_program_metadata *metadata;

#if defined(WORD_SIZE)
    uint16_t num_registers;
    srsvm_register_specification *registers;

    uint16_t num_vmem_segments;
    srsvm_virtual_memory_specification *virtual_memory;
    uint16_t num_lmem_segments;
    srsvm_literal_memory_specification *literal_memory;

    uint16_t num_constants;
    bool constants_compressed;
    size_t constants_original_size;
    size_t constants_compressed_size;

    srsvm_constant_specification *constants;
#endif
} srsvm_program;

uint8_t srsvm_program_word_size(const char* program_path);
void srsvm_program_free(srsvm_program *program);

srsvm_program* srsvm_program_alloc(void);
srsvm_program_metadata* srsvm_program_metadata_alloc(void);
void srsvm_program_free_metadata(srsvm_program_metadata* metadata);

srsvm_program *srsvm_program_deserialize(const char* program_path);
#if defined(WORD_SIZE)
bool srsvm_program_serialize(const char* output_path, const srsvm_program* program);

void srsvm_program_free_register(srsvm_register_specification *reg);
void srsvm_program_free_vmem(srsvm_virtual_memory_specification *vmem);
void srsvm_program_free_lmem(srsvm_literal_memory_specification *lmem);
void srsvm_program_free_const(srsvm_constant_specification *c);

srsvm_register_specification* srsvm_program_register_alloc(void);
srsvm_virtual_memory_specification* srsvm_program_vmem_alloc(void);
srsvm_literal_memory_specification* srsvm_program_lmem_alloc(void);
srsvm_constant_specification* srsvm_program_const_alloc(void);

#endif
