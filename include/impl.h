#pragma once

#include <stdbool.h>

#include "forward-decls.h"

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

typedef bool (*srsvm_module_opcode_loader)(void*, srsvm_opcode*);
bool srsvm_native_module_load(srsvm_native_module_handle* handle, const char* filename);
void srsvm_native_module_unload(srsvm_native_module_handle* handle);
bool srsvm_native_module_load_opcodes(srsvm_native_module_handle* handle, srsvm_module_opcode_loader loader, void* arg);

char* srsvm_module_name_to_filename(const char* module_name);

char* srsvm_getcwd(void);
bool srsvm_path_is_absolute(const char* path);
bool srsvm_directory_exists(const char* dir_name);
bool srsvm_file_exists(const char* file_name);
char *srsvm_path_combine(const char* path_1, const char* path_2);

