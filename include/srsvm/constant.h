#pragma once

#include <stdbool.h>

#include "srsvm/memory.h"
#include "srsvm/register.h"
#include "srsvm/value_types.h"
#include "srsvm/word.h"

#define SRSVM_CONST_MAX_COUNT (WORD_SIZE * 4)

typedef struct
{
    union
    {
        srsvm_word word;

        srsvm_ptr ptr;
        srsvm_ptr_offset ptr_offset;

        bool bit;

        uint8_t u8;
        int8_t i8;

        uint16_t u16;
        int16_t i16;

#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
        uint32_t u32;
        int32_t i32;

        float f32;
#endif

#if WORD_SIZE == 64 || WORD_SIZE == 128
        uint64_t u64;
        int64_t i64;

        double f64;
#endif

#if WORD_SIZE == 128
        unsigned __int128 u128;
        __int128 i128;
#endif
    };

    const char *str;
    srsvm_word str_len;

    srsvm_value_type type;
} srsvm_constant_value;

srsvm_constant_value *srsvm_const_alloc(const srsvm_value_type type);
void srsvm_const_free(srsvm_constant_value* c);
bool srsvm_const_load(srsvm_register *dest_reg, srsvm_constant_value *val, const srsvm_word offset);
