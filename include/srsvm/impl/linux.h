#pragma once

#include <pthread.h>

#define SRSVM_MODULE_FILE_EXTENSION ".svm"

typedef pthread_mutex_t srsvm_lock;

typedef pthread_t srsvm_thread_native_handle;

typedef void* srsvm_native_module_handle;
