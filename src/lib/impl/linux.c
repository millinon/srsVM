#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/limits.h>
#include <libgen.h>
#include <pthread.h>
#include <unistd.h>

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

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    success = pthread_mutex_init(lock, &attr) == 0;

    return success;
}

void srsvm_lock_destroy(srsvm_lock *lock)
{
    pthread_mutex_destroy(lock);
}

bool srsvm_lock_acquire(srsvm_lock *lock)
{
    bool success = false;
    
    success = pthread_mutex_lock(lock) == 0;

    return success;
}

void srsvm_lock_release(srsvm_lock *lock)
{
    pthread_mutex_unlock(lock);
}

#ifdef WORD_SIZE
typedef struct
{
    native_thread_proc proc;
    void* arg;
} wrapped_thread_info;

void *pthread_start_wrapper(void* arg)
{
    wrapped_thread_info *info = arg;

    info->proc(info->arg);

    free(info);

    return NULL;
}

bool srsvm_thread_start(srsvm_thread *thread, native_thread_proc proc, void *arg)
{
    bool success = false;

    wrapped_thread_info *info = malloc(sizeof(wrapped_thread_info));

    if(info != NULL){

        info->proc = proc;
        info->arg = arg;

        if(pthread_create(&thread->native_handle, NULL, pthread_start_wrapper, info) == 0){
            success = true;
        }
    }

    return success;
}

void srsvm_thread_exit(srsvm_thread_exit_info *info)
{
    pthread_exit(info);
}

bool srsvm_thread_join(srsvm_thread *thread, srsvm_thread_exit_info ** ret)
{
    bool success = false;

    if(pthread_join(thread->native_handle, (void**)ret) == 0){
        success = true;
    }

    return success;
}

void srsvm_sleep(const srsvm_word ms_timeout)
{
    struct timespec sleep_time, remaining_time;

    sleep_time.tv_sec = ms_timeout / 1000;
    sleep_time.tv_nsec = (ms_timeout % 1000) * 1000 * 1000;

    do {
        if(nanosleep(&sleep_time, &remaining_time) == 0){
            break;
        }
        sleep_time = remaining_time;
    } while(remaining_time.tv_sec > 0 || remaining_time.tv_nsec > 0);
}

bool srsvm_native_module_load(srsvm_native_module_handle *handle, const char* filename)
{
    bool success = false;

    *handle = dlopen(filename, RTLD_LAZY);

    if(*handle == NULL){
        dbg_printf("load failed: %s\n", dlerror());
    }

    success = *handle != NULL;

    return success;
}

void srsvm_native_module_unload(srsvm_native_module_handle *handle)
{
    if(handle != NULL)
        dlclose(handle);
}

typedef bool (*opcode_enumerator)(srsvm_module_opcode_loader, void*);

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

bool srsvm_native_module_load_opcodes(srsvm_native_module_handle *handle, srsvm_module_opcode_loader loader, void* arg)
{
    bool success = false;

    if(srsvm_native_module_supports_word_size(handle, WORD_SIZE)){
        opcode_enumerator enumerator = dlsym(*handle, "srsvm_enumerate_opcodes_" STR(WORD_SIZE));

        if(enumerator != NULL){
            success = enumerator(loader, arg);
        }
    }

    return success;
}

typedef bool(*word_size_check)(const uint8_t word_size);

bool srsvm_native_module_supports_word_size(srsvm_native_module_handle *handle, const uint8_t word_size)
{
    bool success = false;

    word_size_check check = dlsym(*handle, "srsvm_word_size_support");

    if(check != NULL){
        success = check(word_size);
    }

    return success;
}

char *srsvm_module_name_to_filename(const char* module_name)
{
    size_t name_len = strlen(module_name);
    size_t ext_len = strlen(SRSVM_MODULE_FILE_EXTENSION);

    char* writable_arg = strdup(module_name);
    
    char* path = NULL;

    if(writable_arg != NULL){
        path = malloc((name_len + ext_len + 1) * sizeof(char));
        
        if(path != NULL){
            memset(path, 0, (name_len + ext_len + 4) * sizeof(char));

            strncpy(path, basename(writable_arg), (name_len + 4));
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

    if((path = malloc(PATH_MAX * sizeof(char))) != NULL){
        if(getcwd(path, PATH_MAX) == NULL){
            free(path);
            path = NULL;
        }
    }

    return path;
}

bool srsvm_path_is_absolute(const char* path)
{
    bool is_absolute = false;

    char* resolved = realpath(path, NULL);

    if(resolved != NULL){
        if(strcmp(path, resolved) == 0){
            is_absolute = true;
        }

        free(resolved);
    }

    return is_absolute;
}

bool srsvm_directory_exists(const char* dir_name)
{
    bool dir_exists = false;

    struct stat s_buf;
    if(stat(dir_name, &s_buf) == 0){
        dir_exists = S_ISDIR(s_buf.st_mode) != 0;
    }

    return dir_exists;
}

bool srsvm_file_exists(const char* file_name)
{
    bool file_exists = false;

    struct stat s_buf;
    if(stat(file_name, &s_buf) == 0){
        file_exists = S_ISREG(s_buf.st_mode) != 0;
    }

    return file_exists;
}

char* srsvm_path_combine(const char* path_1, const char* path_2)
{
    size_t len_1 = strlen(path_1);
    size_t len_2 = strlen(path_2);

    char* combined = malloc((len_1 + len_2 + 2) * sizeof(char));
    
    if(combined != NULL){
        memset(combined, 0, (len_1 + len_2 + 2) * sizeof(char));

        strcat(combined, path_1);
        strcat(combined, "/");
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
    return strcasecmp(a, b);
}

int srsvm_strncasecmp(const char* a, const char* b, const size_t count)
{
    return strncasecmp(a, b, count);
}

char *srsvm_strdup(const char* s)
{
	return strdup(s);
}

char *srsvm_strndup(const char* s, const size_t n)
{
	return strndup(s, n);
}
