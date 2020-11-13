#pragma once

#include <inttypes.h>
#include <stdint.h>

#if WORD_SIZE == 16

typedef uint16_t srsvm_word;

#define SWF "%" PRIu16
#define SWFX "0x%" PRIx16

#define SW_PARAM(w) (w)

#elif WORD_SIZE == 32

typedef uint32_t srsvm_word;

#define SWF "%" PRIu32
#define SWFX "0x" PRIx32

#define SW_PARAM(w) (w)

#elif WORD_SIZE == 64

typedef uint64_t srsvm_word;

#define SWF "%" PRIu64
#define SWFX "0x" PRIx64

#define SW_PARAM(w) (w)

#elif WORD_SIZE == 128

#ifdef __SIZEOF_INT128__

typedef unsigned __int128 srsvm_word;

#define SWF "0x" "%" PRIx64 "%" PRIx64
#define SWFX SWF

#define SW_PARAM(w) ((uint64_t)(w >> 64)),((uint64_t)(w & 0xFFFFFFFFFFFFFFF))

#else

#error "This compiler does not support 128-bit integers"

#endif

#else

#error "WORD_SIZE not defined or has an invalid value"

#endif
