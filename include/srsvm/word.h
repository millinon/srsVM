#pragma once

#include <inttypes.h>
#include <stdint.h>

#if WORD_SIZE == 16

typedef uint16_t srsvm_word;

#define PRINT_WORD "%" PRIu16
#define PRINT_WORD_HEX "0x%" PRIx16

#define PRINTF_WORD_PARAM(w) (w)

#define SCAN_WORD_HEX "%"  SCNx16

#elif WORD_SIZE == 32

typedef uint32_t srsvm_word;

#define PRINT_WORD "%" PRIu32
#define PRINT_WORD_HEX "0x%" PRIx32

#define PRINTF_WORD_PARAM(w) (w)

#define SCAN_WORD_HEX "%"  SCNx32

#elif WORD_SIZE == 64

typedef uint64_t srsvm_word;

#define PRINT_WORD "%" PRIu64
#define PRINT_WORD_HEX "0x%" PRIx64

#define PRINTF_WORD_PARAM(w) (w)

#define SCAN_WORD_HEX "%"  SCNx64

#elif WORD_SIZE == 128

#ifdef __SIZEOF_INT128__

typedef unsigned __int128 srsvm_word;

#define PRINT_WORD "0x" "%" PRIx64 "%" PRIx64
#define PRINT_WORD_HEX PRINT_WORD

#define PRINTF_WORD_PARAM(w) ((uint64_t)(((srsvm_word) w) >> 64)),((uint64_t)(((srsvm_word) w) & 0xFFFFFFFFFFFFFFFull))

#else

#error "This compiler does not support 128-bit integers"

#endif

#else

#error "WORD_SIZE not defined or has an invalid value"

#endif
