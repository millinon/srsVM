#pragma once

#include <stdbool.h>

#include "forward-decls.h"

#include "word.h"
#include "impl.h"

#if WORD_SIZE == 16

typedef uint16_t srsvm_ptr;
typedef int16_t srsvm_ptr_offset;

#define SRSVM_NULL_PTR 0
#define SRSVM_MAX_PTR UINT16_MAX

#elif WORD_SIZE == 32

typedef uint32_t srsvm_ptr;
typedef int32_t srsvm_ptr_offset;

#define SRSVM_NULL_PTR 0
#define SRSVM_MAX_PTR UINT32_MAX

#elif WORD_SIZE == 64

typedef uint64_t srsvm_ptr;
typedef int64_t srsvm_ptr_offset;

#define SRSVM_NULL_PTR 0
#define SRSVM_MAX_PTR UINT64_MAX

#elif WORD_SIZE == 128

typedef unsigned __int128 srsvm_ptr;
typedef __int128 srsvm_ptr_offset;

#define SRSVM_NULL_PTR 0
#define SRSVM_MAX_PTR UINT64_MAX

#endif

struct srsvm_memory_segment
{
    struct srsvm_memory_segment *parent;    

    srsvm_ptr min_address;
    srsvm_ptr max_address;
    srsvm_word sz;

    srsvm_word level;

    void *literal_memory;
    srsvm_ptr literal_start;
    srsvm_word literal_sz;

    srsvm_memory_segment *children[WORD_SIZE];
    
    bool readable;
    bool writable;
    bool executable;
    
    bool locked;

    bool free_flag;

    srsvm_lock lock;
};

typedef struct srsvm_virtual_memory_desc srsvm_virtual_memory_desc; 

struct srsvm_virtual_memory_desc
{
    srsvm_ptr start_address;
    srsvm_word size;

    srsvm_virtual_memory_desc *next;
};

srsvm_virtual_memory_desc *srsvm_virtual_memory_layout(srsvm_virtual_memory_desc* last, const srsvm_ptr start_address, const srsvm_word size);

void srsvm_virtual_memory_layout_free(srsvm_virtual_memory_desc* list);
