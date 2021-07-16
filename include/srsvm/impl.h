#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "srsvm/forward-decls.h"

#if defined(WORD_SIZE)
#include "srsvm/word.h"
#endif

#include "srsvm/config.h"


#if defined(SRSVM_BUILD_TARGET_LINUX)

#include "srsvm/impl/linux.h"

#elif defined(SRSVM_BUILD_TARGET_WINDOWS)

#include "srsvm/impl/windows.h"

#else

#error "Unsupported platform"

#endif

bool srsvm_lock_initialize(srsvm_lock *lock);
void srsvm_lock_destroy(srsvm_lock *lock);

bool srsvm_lock_acquire(srsvm_lock *lock, const long ms_timeout);
void srsvm_lock_release(srsvm_lock *lock);

#if defined(WORD_SIZE)
typedef void (*native_thread_proc)(void*);

bool srsvm_thread_start(srsvm_thread *thread, native_thread_proc proc, void* arg);
void srsvm_thread_exit(srsvm_thread *thread);
bool srsvm_thread_join(srsvm_thread *thread);

void srsvm_sleep(const srsvm_word ms_timeout);

typedef bool (*srsvm_module_opcode_loader)(void*, srsvm_opcode*);

bool srsvm_native_module_load_opcodes(srsvm_native_module_handle* handle, srsvm_module_opcode_loader loader, void* arg);
#endif

bool srsvm_native_module_supports_word_size(srsvm_native_module_handle *handle, const uint8_t word_size);

bool srsvm_native_module_load(srsvm_native_module_handle* handle, const char* filename);
void srsvm_native_module_unload(srsvm_native_module_handle* handle);

char* srsvm_module_name_to_filename(const char* module_name);

char* srsvm_getcwd(void);
bool srsvm_path_is_absolute(const char* path);
bool srsvm_directory_exists(const char* dir_name);
bool srsvm_file_exists(const char* file_name);
char *srsvm_path_combine(const char* path_1, const char* path_2);

int srsvm_strcasecmp(const char* a, const char* b);
int srsvm_strncasecmp(const char* a, const char* b, const size_t count);

char *srsvm_strdup(const char* s);
char *srsvm_strndup(const char* s, const size_t n);

#if defined(SRSVM_SUPPORT_COMPRESSION)
void *srsvm_zlib_deflate(const void* data, size_t *compressed_size, const size_t original_size);
void *srsvm_zlib_inflate(const void* data, const size_t compressed_size, const size_t original_size);
#endif
