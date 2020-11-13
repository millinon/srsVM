#pragma once

#include <stdbool.h>

#include "forward-decls.h"
#include "word.h"

#ifdef __unix__

#include "impl/linux.h"

#else

#error "Unsupported platform"

#endif

bool srsvm_lock_initialize(srsvm_lock *lock);
void srsvm_lock_destroy(srsvm_lock *lock);

bool srsvm_lock_acquire(srsvm_lock *lock, const long ms_timeout);
void srsvm_lock_release(srsvm_lock *lock);

typedef void (*native_thread_proc)(void*);

bool srsvm_thread_start(srsvm_thread *thread, native_thread_proc proc, void* arg);
void srsvm_thread_exit(srsvm_thread *thread);
bool srsvm_thread_join(srsvm_thread *thread);

void srsvm_sleep(const long ms_timeout);
