#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "srsvm/impl.h"

#define SRSVM_HANDLE_MAX_COUNT (8 * WORD_SIZE)

typedef enum
{
    SRSVM_HANDLE_TYPE_FILE = 0,

    SRSVM_HANDLE_TYPE_BUF_FILE = 1,

    SRSVM_HANDLE_TYPE_MUTEX = 2,
    
    SRSVM_HANDLE_TYPE_THREAD = 3,
} srsvm_handle_type;

struct srsvm_handle
{
    bool is_open;
    bool has_error;

    union {
        srsvm_lock mutex;
        srsvm_thread *thread;
        #ifdef _WIN32
        HANDLE hnd;
        #else
        int fd;

        FILE *buf_file;

        #endif
    };

    srsvm_handle_type type;
};

srsvm_handle *srsvm_handle_alloc(const srsvm_handle_type type);
void srsvm_handle_free(srsvm_handle* h);
bool srsvm_handle_close(srsvm_handle* h);
