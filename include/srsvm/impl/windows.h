#pragma once

#include <Windows.h>

#define SRSVM_MODULE_FILE_EXTENSION ".svmmod"

#if defined(SRSVM_INTERNAL)
#define SRSVM_EXPORT __declspec(dllexport)
#else
#define SRSVM_EXPORT extern
#endif

#define SRSVM_MAX_PATH_LEN MAX_PATH

#define srsvm_strncpy(dst,src,len) strncpy_s(dst,sizeof(dst),src,len)

typedef CRITICAL_SECTION srsvm_lock;

typedef HANDLE srsvm_thread_native_handle;

typedef HANDLE srsvm_native_module_handle;

