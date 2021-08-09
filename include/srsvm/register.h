#pragma once

#include <stdbool.h>

#include "srsvm/memory.h"
#include "srsvm/word.h"

#define SRSVM_REGISTER_MAX_COUNT (8 * WORD_SIZE)
#define SRSVM_REGISTER_MAX_NAME_LEN 256

typedef struct
{
    union
    {
        srsvm_word word;
        
        srsvm_ptr ptr;
        srsvm_ptr_offset ptr_offset;

        bool bit;

#if WORD_SIZE == 16
        uint16_t u16[1];
        int16_t i16[1];

        uint8_t u8[2];
        int8_t i8[2];
#elif WORD_SIZE == 32
        uint32_t u32[1];
        int32_t i32[1];

        float f32[1];

        uint16_t u16[2];
        int16_t i16[2];

        uint8_t u8[4];
        int8_t i8[4];
#elif WORD_SIZE == 64
        uint64_t u64[1];
        int64_t i64[1];

        double f64[1];

        uint32_t u32[2];
        int32_t i32[2];

        float f32[2];

        uint16_t u16[4];
        int16_t i16[4];

        uint8_t u8[8];
        int8_t i8[8];
#elif WORD_SIZE == 128        
        unsigned __int128 u128[1];
        __int128 i128[1];

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
#endif
    };
    char* str;
    size_t str_len;
} srsvm_register_contents;

typedef struct
{
    srsvm_word index;

    char name[SRSVM_REGISTER_MAX_NAME_LEN];

    bool read_only;
    
    bool locked;

    bool error_flag;
    char error_str[1024];

    srsvm_register_contents value;
} srsvm_register;

srsvm_register *srsvm_register_alloc(const char* name, const srsvm_word index);
void srsvm_register_free(srsvm_register *reg);
