#pragma once

#include <stdbool.h>

#include "srsvm/word.h"

typedef enum
{
    SRSVM_TYPE_WORD = 0,

	SRSVM_TYPE_PTR = 1,
	SRSVM_TYPE_PTR_OFFSET = 2,

	SRSVM_TYPE_BIT = 3,

	SRSVM_TYPE_STR = 4,

	SRSVM_TYPE_U8 = 5,
	SRSVM_TYPE_I8 = 6,

	SRSVM_TYPE_U16 = 7,
	SRSVM_TYPE_I16 = 8,

#if WORD_SIZE == 16
    SRSVM_MAX_TYPE_VALUE = 8,
#endif

#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
	SRSVM_TYPE_U32 = 9,
	SRSVM_TYPE_I32 = 10,

	SRSVM_TYPE_F32 = 11,
#endif

#if WORD_SIZE == 32
    SRSVM_MAX_TYPE_VALUE = 11,
#endif

#if WORD_SIZE == 64 || WORD_SIZE == 128
	SRSVM_TYPE_U64 = 12,
	SRSVM_TYPE_I64 = 13,

	SRSVM_TYPE_F64 = 14,
#endif

#if WORD_SIZE == 64
    SRSVM_MAX_TYPE_VALUE = 14,
#endif

#if WORD_SIZE ==  128
	SRSVM_TYPE_U128 = 15,
	SRSVM_TYPE_I128 = 16,

    SRSVM_MAX_TYPE_VALUE = 16,
#endif
} srsvm_value_type;
