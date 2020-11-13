#pragma once

#include <stdbool.h>

#include "word.h"

#include "memory.h"


#if WORD_SIZE == 16

typedef struct
{
    union
    {
        srsvm_ptr ptr;

        uint16_t u16;
        int16_t i16;

        uint8_t u8[2];
        int8_t i8[2];
    };
    char* str;
    size_t str_len;
} srsvm_register_contents;

#elif WORD_SIZE == 32

typedef struct
{
    union
    {
        srsvm_ptr ptr;

        uint32_t u32;
        int32_t i32;

        float f32;

        uint16_t u16[2];
        int16_t i16[2];

        uint8_t u8[4];
        int8_t i8[4];
    };
    char* str;
    size_t str_len;
} srsvm_register_contents;

#elif WORD_SIZE == 64

typedef struct
{
    union
    {
        srsvm_ptr ptr;

        uint64_t u64;
        int64_t i64;

        double f64;

        uint32_t u32[2];
        int32_t i32[2];

        float f32[2];

        uint16_t u16[4];
        int16_t i16[4];

        uint8_t u8[8];
        int8_t i8[8];
    };
    char* str;
    size_t str_len;
} srsvm_register_contents;

#elif WORD_SIZE == 128

typedef struct
{
    union
    {
        srsvm_ptr ptr;

        unsigned __int128 u128;
        __int128 i128;

        uint64_t u64[2];
        int64_t i64[2];

        double f64[2];

        uint32_t u32[4];
        int32_t i32[4];

        float f32[4];

        uint16_t u16[8];
        int16_t i16[8];

        uint8_t u8[16];
        int8_t i8[16];
    };
    char* str;
    size_t str_len;
} srsvm_register_contents;

#else

#error "WORD_SIZE not defined or has an invalid value"

#endif

typedef struct
{
    srsvm_word index;

    char name[256];

    bool read_only;
    
    bool locked;

    bool error_flag;
    const char* error_str;

    srsvm_register_contents value;
} srsvm_register;

srsvm_register *srsvm_register_alloc(const char* name, const srsvm_word index);

#define MAX_REGISTER_COUNT (8 * WORD_SIZE)
