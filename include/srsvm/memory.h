#pragma once

#include <stdbool.h>

#include "srsvm/forward-decls.h"

#include "srsvm/impl.h"
#include "srsvm/word.h"

#if WORD_SIZE == 16

//typedef uint16_t srsvm_ptr;
//typedef int16_t srsvm_ptr_offset;

#define SRSVM_NULL_PTR 0
#define SRSVM_MAX_PTR UINT16_MAX

#define SRSVM_MIN_PTR_OFF INT16_MIN
#define SRSVM_MAX_PTR_OFF INT16_MAX

#elif WORD_SIZE == 32

//typedef uint32_t srsvm_ptr;
//typedef int32_t srsvm_ptr_offset;

#define SRSVM_NULL_PTR 0
#define SRSVM_MAX_PTR UINT32_MAX

#define SRSVM_MIN_PTR_OFF INT32_MIN
#define SRSVM_MAX_PTR_OFF INT32_MAX

#elif WORD_SIZE == 64

//typedef uint64_t srsvm_ptr;
//typedef int64_t srsvm_ptr_offset;

#define SRSVM_NULL_PTR 0
#define SRSVM_MAX_PTR UINT64_MAX

#define SRSVM_MIN_PTR_OFF INT64_MIN
#define SRSVM_MAX_PTR_OFF INT64_MAX

#elif WORD_SIZE == 128

//typedef unsigned __int128 srsvm_ptr;
//typedef __int128 srsvm_ptr_offset;

#define SRSVM_NULL_PTR 0
#define SRSVM_MAX_PTR ((\
            ((unsigned __int128) 0xFFFFFFFFFFFFFFFFull) << 64) | \
            ((unsigned __int128) 0xFFFFFFFFFFFFFFFFull))

#define SRSVM_MAX_PTR_OFF (((__int128)0x7FFFFFFFFFFFFFFFull << 64) + 0xFFFFFFFFFFFFFFFFull)
#define SRSVM_MIN_PTR_OFF (((__int128)-1) - SRSVM_MAX_PTR_OFF)

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
