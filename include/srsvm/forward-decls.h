#pragma once

#include <stdint.h>

#ifdef WORD_SIZE

#if WORD_SIZE == 16
    typedef uint16_t srsvm_ptr;
    typedef int16_t srsvm_ptr_offset;
#elif WORD_SIZE == 32
    typedef uint32_t srsvm_ptr;
    typedef int32_t srsvm_ptr_offset;
#elif WORD_SIZE == 64
    typedef uint64_t srsvm_ptr;
    typedef int64_t srsvm_ptr_offset;
#elif WORD_SIZE == 128
    typedef unsigned __int128 srsvm_ptr;
    typedef __int128 srsvm_ptr_offset;
#endif

#endif

typedef struct srsvm_vm srsvm_vm;

typedef struct srsvm_memory_segment srsvm_memory_segment;

typedef struct srsvm_thread_exit_info srsvm_thread_exit_info;

typedef struct srsvm_thread srsvm_thread;

typedef struct srsvm_handle srsvm_handle;

typedef struct srsvm_opcode srsvm_opcode;

typedef struct srsvm_opcode_map srsvm_opcode_map;
