#pragma once

#include <pthread.h>

typedef pthread_mutex_t srsvm_lock;

typedef pthread_t srsvm_thread_native_handle;

typedef void* srsvm_native_module_handle;