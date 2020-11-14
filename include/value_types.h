#pragma once

#include <stdbool.h>

#include "word.h"

typedef enum
{
    WORD = 0,

    PTR = 1,
    PTR_OFFSET = 2,

    BIT = 3,

    STR = 4,

    U8 = 5,
    I8 = 6,

    U16 = 7,
    I16 = 8,

#if WORD_SIZE == 16
    MAX_TYPE_VALUE = 8,
#endif

#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
    U32 = 9,
    I32 = 10,

    F32 = 11,
#endif

#if WORD_SIZE == 32
    MAX_TYPE_VALUE = 11,
#endif

#if WORD_SIZE == 64 || WORD_SIZE == 128
    U64 = 12,
    I64 = 13,

    F64 = 14,
#endif

#if WORD_SIZE == 64
    MAX_TYPE_VALUE = 14,
#endif

#if WORD_SIZE ==  128
    U128 = 15,
    I128 = 16,

    MAX_TYPE_VALUE = 16,
#endif
} srsvm_value_type;
