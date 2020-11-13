#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "register.h"

srsvm_register *srsvm_register_alloc(const char* name, const srsvm_word index)
{
    dbg_printf("allocating register %s at index " SWF, name, SW_PARAM(index));

    srsvm_register *reg = NULL;

    reg = malloc(sizeof(srsvm_register));

    if(reg != NULL){
        reg->index = index;
        strncpy(reg->name, name, sizeof(reg->name));
        reg->error_flag = false;
        reg->read_only = false;
        reg->locked = false;

        memset(&reg->value, 0, sizeof(srsvm_register_contents));
        reg->value.str = NULL;
        reg->value.str_len = 0;
    } else {
        dbg_printf("malloc failed: %s", strerror(errno));
    }

    return reg;
}
