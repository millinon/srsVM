#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <Windows.h>
#include <shlwapi.h>

#include "srsvm/config.h"
#include "srsvm/debug.h"
#include "srsvm/impl.h"
#if defined(WORD_SIZE)
#include "srsvm/thread.h"
#endif

#if defined(SRSVM_SUPPORT_COMPRESSION)
#include <zlib.h>
#endif

bool srsvm_lock_initialize(srsvm_lock *lock)
{
    bool success = false;

    InitializeCriticalSection(lock);

	success = true;

    return success;
}

void srsvm_lock_destroy(srsvm_lock *lock)
{
    DeleteCriticalSection(lock);
}

bool srsvm_lock_acquire(srsvm_lock *lock)
{
    EnterCriticalSection(lock);
	
    return true;
}

void srsvm_lock_release(srsvm_lock *lock)
{
    LeaveCriticalSection(lock);
}

#ifdef WORD_SIZE
typedef struct
{
    native_thread_proc proc;
    void* arg;
} wrapped_thread_info;

DWORD WINAPI createthread_wrapper(LPVOID arg)
{
    wrapped_thread_info *info = arg;

    info->proc(info->arg);

    free(info);

    return 0;
}

bool srsvm_thread_start(srsvm_thread *thread, native_thread_proc proc, void *arg)
{
    bool success = false;

    wrapped_thread_info *info = malloc(sizeof(wrapped_thread_info));

    if(info != NULL){
        info->proc = proc;
        info->arg = arg;

		success = (thread->native_handle = CreateThread(NULL, 0, createthread_wrapper, info, 0, NULL)) != NULL;

		if (!success) {
			free(info);
		}
    }

    return success;
}

void srsvm_thread_exit(srsvm_thread *thread)
{
	ExitThread(0);
}

bool srsvm_thread_join(srsvm_thread *thread)
{
    bool success = false;

	DWORD join_result = WaitForSingleObject(thread->native_handle, INFINITE);

	success = join_result == WAIT_OBJECT_0;

    return success;
}

void srsvm_sleep(const srsvm_word ms_timeout)
{
	Sleep((DWORD)ms_timeout);
}

bool srsvm_native_module_load(srsvm_native_module_handle *handle, const char* filename)
{
    bool success = false;

	*handle = LoadLibrary(filename);

    success = *handle != NULL;

    return success;
}

void srsvm_native_module_unload(srsvm_native_module_handle *handle)
{
    if(handle != NULL)
        FreeLibrary(*handle);
}

typedef bool (*opcode_enumerator)(srsvm_module_opcode_loader, void*);

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

bool srsvm_native_module_load_opcodes(srsvm_native_module_handle *handle, srsvm_module_opcode_loader loader, void* arg)
{
    bool success = false;

    if(srsvm_native_module_supports_word_size(handle, WORD_SIZE)){
        opcode_enumerator enumerator = (opcode_enumerator) GetProcAddress(*handle, "srsvm_enumerate_opcodes_" STR(WORD_SIZE));

        if(enumerator != NULL){
            success = enumerator(loader, arg);

			if (!success) {
				dbg_printf("opcode enumerator failed");
			}
		}
		else {
			dbg_puts("failed to enumerate opcodes");
		}
	}
	else {
		dbg_puts("mod unsupported 2");
	}

    return success;
}

typedef bool(*word_size_check)(const uint8_t word_size);

bool srsvm_native_module_supports_word_size(srsvm_native_module_handle *handle, const uint8_t word_size)
{
    bool success = false;

    word_size_check check = (word_size_check) GetProcAddress(*handle, "srsvm_word_size_support");

    if(check != NULL){
        success = check(word_size);
    }

    return success;
}

char *srsvm_module_name_to_filename(const char* module_name)
{
    size_t name_len = strlen(module_name);
    size_t ext_len = strlen(SRSVM_MODULE_FILE_EXTENSION);

    char* writable_arg = _strdup(module_name);
    
    char* path = NULL;

    if(writable_arg != NULL){
		PathStripPath(writable_arg);
		
		path = malloc((name_len + ext_len + 1) * sizeof(char));
        
        if(path != NULL){
            memset(path, 0, (name_len + ext_len + 4) * sizeof(char));

            strncpy(path, writable_arg, (name_len + 4));
            strncat(path, SRSVM_MODULE_FILE_EXTENSION, ext_len);             

            free(writable_arg);
        } 
    }

    return path;
}
#endif

char* srsvm_getcwd(void)
{
    char *path = NULL;

    if((path = malloc(SRSVM_MAX_PATH_LEN * sizeof(char))) != NULL){
		if (GetCurrentDirectory(SRSVM_MAX_PATH_LEN * sizeof(char), path) == 0) {
			free(path);
			path = NULL;
		}
    }

    return path;
}

bool srsvm_path_is_absolute(const char* path)
{
    bool is_absolute = false;

	is_absolute = !PathIsRelative(path);

    return is_absolute;
}

bool srsvm_directory_exists(const char* dir_name)
{
	DWORD dwAttrib = GetFileAttributes(dir_name);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool srsvm_file_exists(const char* file_name)
{
	bool dir_exists = false;

	DWORD dwAttrib = GetFileAttributes(file_name);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

char* srsvm_path_combine(const char* path_1, const char* path_2)
{
    size_t len_1 = strlen(path_1);
    size_t len_2 = strlen(path_2);

    char* combined = malloc((len_1 + len_2 + 2) * sizeof(char));
    
    if(combined != NULL){
        memset(combined, 0, (len_1 + len_2 + 2) * sizeof(char));

        strcat(combined, path_1);
        strcat(combined, "\\");
        strcat(combined, path_2);
    }

    return combined;
}

#if defined(SRSVM_SUPPORT_COMPRESSION)
void *srsvm_zlib_deflate(const void* data, size_t *compressed_size, const size_t original_size)
{
    void *deflated_data = NULL;

    unsigned long source_len = original_size;
    
    int min_size = compressBound(source_len);

    if(min_size <= 0){
        goto error_cleanup;
    }

    deflated_data = malloc(min_size);

    if(deflated_data != NULL){
        unsigned long dest_len = min_size;

        int compress_result = compress(deflated_data, &dest_len, data, source_len);
   
        switch(compress_result){
            case Z_OK:
                break;

            case Z_MEM_ERROR:
                dbg_puts("ERROR: failed to decompress memory: not enough memory");
                goto error_cleanup;

            case Z_BUF_ERROR:
                dbg_puts("ERROR: failed to decompress memory: not enough room in output buffer");
                goto error_cleanup;

            default:
                dbg_puts("ERROR: failed to decompress memory: unknown error");
                goto error_cleanup;
        }

        *compressed_size = (size_t) dest_len;
    }

    return deflated_data;

error_cleanup:
    if(deflated_data != NULL){
        free(deflated_data);
    }

    return NULL;
}

void* srsvm_zlib_inflate(const void* data, const size_t compressed_size, const size_t original_size)
{
    void* inflated_data = malloc(original_size);

    if(inflated_data != NULL){
        unsigned long dest_len = original_size;
        unsigned long source_len = compressed_size;

        int uncompress_result = uncompress(inflated_data, &dest_len, data, source_len);

        switch(uncompress_result){
            case Z_OK:
                break;

            case Z_MEM_ERROR:
                dbg_puts("ERROR: failed to decompress memory: not enough memory");
                goto error_cleanup;

            case Z_BUF_ERROR:
                dbg_puts("ERROR: failed to decompress memory: not enough room in output buffer");
                goto error_cleanup;

            case Z_DATA_ERROR:
                dbg_puts("ERROR: failed to decompress memory: data is corrupted");
                goto error_cleanup;

            default:
                dbg_puts("ERROR: failed to decompress memory: unknown error");
                goto error_cleanup;
        }
    }
    
    return inflated_data;

error_cleanup:
    if(inflated_data != NULL){
        free(inflated_data);
    }

    return NULL;
}
#endif

int srsvm_strcasecmp(const char* a, const char* b)
{
    return _stricmp(a, b);
}

int srsvm_strncasecmp(const char* a, const char* b, const size_t count)
{
    return _strnicmp(a, b, count);
}

char* srsvm_strdup(const char* s)
{
    return _strdup(s);
}

char* srsvm_strndup(const char* s, const size_t n)
{
    return _strdup(s);
}
