#pragma once

#ifndef NDEBUG

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>

extern bool srsvm_debug_mode;

#define dbg_printf(fmt, ...) do { if(srsvm_debug_mode) fprintf(stderr, "[DEBUG] " fmt "\n",  __VA_ARGS__); } while(0)
#define dbg_puts(s) do { if(srsvm_debug_mode) dbg_printf("%s", s); } while(0)

#else

#define dbg_printf(fmt, ...) do { } while(0)
#define dbg_puts(s) do { } while(0)

#endif
