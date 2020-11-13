#include <stdlib.h>
#include <time.h>

#include <pthread.h>

#include "impl.h"

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

