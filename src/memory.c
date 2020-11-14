#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "memory.h"

srsvm_virtual_memory_desc *srsvm_virtual_memory_layout(srsvm_virtual_memory_desc* list, const srsvm_ptr start_address, const srsvm_word size)
{
    if(list == NULL){
        dbg_printf("allocating virtual_memory_layout list, start address: " SWFX ", size: " SWF, SW_PARAM(start_address), SW_PARAM(size));
    } else {
        dbg_printf("appending to virtual_memory_layout list %p, start address: " SWFX ", size: " SWF, list, SW_PARAM(start_address), SW_PARAM(size));
    }

    srsvm_virtual_memory_desc * desc;

    desc = malloc(sizeof(srsvm_virtual_memory_desc));
    
    if(desc != NULL){
        if(list != NULL){
            while(list->next != NULL){
                list = list->next;
            }

            list->next = desc;
        }

        desc->start_address = start_address;
        desc->size = size;
        desc->next = NULL;
    } else {
        dbg_printf("malloc failed: %s", strerror(errno));
    }

    return desc;
}

void srsvm_virtual_memory_layout_free(srsvm_virtual_memory_desc* list)
{
    srsvm_virtual_memory_desc *next;
    while(list != NULL){
        next = list->next;
        free(list);
        list = next;
    }
}
