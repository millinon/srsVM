#pragma once

#include <errno.h>

#ifdef DEBUG

#include <stdio.h>

#define dbg_printf(fmt, ...) do { fprintf(stderr, "[DEBUG] " fmt "\n",  __VA_ARGS__); } while(0)
#define dbg_puts(s) do { dbg_printf("%s", s); } while(0)

#else

#define dbg_printf(fmt, ...) do { } while(0)
#define dbg_puts(s) do { } while(0)

#endif
