#include <stdlib.h>
#include <string.h>

#include "srsvm/debug.h"
#include "srsvm/register.h"

srsvm_register *srsvm_register_alloc(const char* name, const srsvm_word index)
{
    dbg_printf("allocating register %s at index " PRINT_WORD, name, PRINTF_WORD_PARAM(index));

    srsvm_register *reg = NULL;

    reg = malloc(sizeof(srsvm_register));

    if(reg != NULL){
        reg->index = index;
		srsvm_strncpy(reg->name, name, sizeof(reg->name) - 1);
        reg->error_flag = false;
        reg->read_only = false;
        reg->locked = false;
	reg->fault_on_error = false;

        memset(&reg->value, 0, sizeof(srsvm_register_contents));
        reg->value.str = NULL;
        reg->value.str_len = 0;

    } else {
        dbg_printf("malloc failed: %s", strerror(errno));
    }

    return reg;
}

void srsvm_register_free(srsvm_register *reg)
{
    if(reg != NULL){
        if(reg->value.str != NULL){
            free(reg->value.str);
        }

        free(reg);
    }
}
