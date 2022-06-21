#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#include <Windows.h>

#else

#include <unistd.h>

#endif

#include "srsvm/debug.h"
#include "srsvm/handle.h"

srsvm_handle *srsvm_handle_alloc(const srsvm_handle_type type)
{
    srsvm_handle *h = malloc(sizeof(srsvm_handle));

    if(h != NULL){
        memset(h, 0, sizeof(srsvm_handle));

        h->is_open = false;
        h->has_error = false;

        h->type = type;
    }

    return h;
}

bool srsvm_handle_close(srsvm_handle *h)
{
    bool success = false;

    if(h != NULL && h->is_open){
        switch(h->type){
            case SRSVM_HANDLE_TYPE_FILE:
#ifdef _WIN32
                if(CloseHandle(h->hnd) != 0){
                    dbg_printf("failed to close handle"); 
                } else success = true;
#else
                if(close(h->fd) == -1){
                    dbg_printf("failed to close handle %d: %s", h->fd, strerror(errno));
                } else success = true;
#endif
                break;

            case SRSVM_HANDLE_TYPE_BUF_FILE:
                if(fclose(h->buf_file) != 0){
                    dbg_printf("failed to close file: %s", strerror(errno));
                } else success = true;
                break;

            case SRSVM_HANDLE_TYPE_MUTEX:
                srsvm_lock_release(&h->mutex);
                success = true;
                break;

            default:
                dbg_printf("cannot close unknown handle type %d", h->type);
                break;
        }
    } else if(h != NULL){
        success = true;
    }

    return success;
}

void srsvm_handle_free(srsvm_handle *h)
{
    srsvm_handle_close(h);

    if(h != NULL){
        free(h);
    }
}
