#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libgen.h>
#include <pthread.h>
#include <unistd.h>

#include "impl.h"

#include "module.h"
#include "thread.h"

bool srsvm_lock_initialize(srsvm_lock *lock)
{
    bool success = false;

    success = pthread_mutex_init(lock, NULL) == 0;

    return success;
}

void srsvm_lock_destroy(srsvm_lock *lock)
{
    pthread_mutex_destroy(lock);
}

bool srsvm_lock_acquire(srsvm_lock *lock, const long ms_timeout)
{
    bool success = false;

    if(ms_timeout > 0){
        struct timespec timeout;
        timeout.tv_sec = ms_timeout / 1000;
        timeout.tv_nsec = (ms_timeout % 1000) * 1000000;

        success = pthread_mutex_timedlock(lock, &timeout) == 0;
    } else {
        success = pthread_mutex_lock(lock) == 0;
    }

    return success;
}

void srsvm_lock_release(srsvm_lock *lock)
{
    pthread_mutex_unlock(lock);
}

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

void srsvm_thread_exit(srsvm_thread *thread)
{
    pthread_exit(NULL);
}

bool srsvm_thread_join(srsvm_thread *thread)
{
    bool success = false;

    if(pthread_join(thread->native_handle, NULL) == 0){
        success = true;
    }

    return success;
}

void srsvm_sleep(const long ms_timeout)
{
    struct timespec sleep_time, remaining_time;

    sleep_time.tv_sec = ms_timeout / 1000;
    sleep_time.tv_nsec = (ms_timeout % 1000) * 1000 * 1000;

    do {
        nanosleep(&sleep_time, &remaining_time);
        sleep_time = remaining_time;
    } while(remaining_time.tv_sec > 0 || remaining_time.tv_nsec > 0);
}

bool srsvm_native_module_load(srsvm_native_module_handle *handle, const char* filename)
{
    bool success = false;

    *handle = dlopen(filename, RTLD_NOW);

    success = *handle != NULL;

    return success;
}

void srsvm_native_module_unload(srsvm_native_module_handle *handle)
{
    dlclose(*handle);
}

typedef bool (*opcode_enumerator)(srsvm_module_opcode_loader, void*);

bool srsvm_native_module_load_opcodes(srsvm_native_module_handle *handle, srsvm_module_opcode_loader loader, void* arg)
{
    bool success = false;

    opcode_enumerator enumerator = dlsym(handle, "enumerate_srsvm_opcodes");

    if(enumerator != NULL){
        success = enumerator(loader, arg);
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
        dir_exists = S_ISDIR(s_buf.st_mode) == 0;
    }

    return dir_exists;
}

bool srsvm_file_exists(const char* file_name)
{
    bool file_exists = false;

    struct stat s_buf;
    if(stat(file_name, &s_buf) == 0){
        file_exists = S_ISREG(s_buf.st_mode) == 0;
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

        strncat(combined, path_1, len_1);
        strcat(combined, "/");
        strncat(combined, path_2, len_2);
    }

    return combined;
}
