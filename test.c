#include <stdio.h>

#include "debug.h"
#include "mmu.h"
#include "vm.h"

int main(){
    srsvm_virtual_memory_desc *list = NULL;

    list = srsvm_virtual_memory_layout(NULL, 0x1000, 512);

    srsvm_vm *vm = srsvm_vm_alloc(list);
    
    if(vm == NULL){
        printf("oh no\n");
        return 1;
    }

    srsvm_memory_segment *progdata = srsvm_mmu_alloc_literal(vm->mem_root, 16, 0x1000);

    if(progdata == NULL){
        printf("yikes\n");
        return 1;
    } else {
        printf("allocated page: start = 0x%lx, literal_start=0x%lx size = %lu\n", progdata->min_address, progdata->literal_start, progdata->literal_sz);
    }

    srsvm_word program[] = {
        0,
        1,
    };

    if(! srsvm_mmu_store(progdata, progdata->literal_start, 2 * sizeof(srsvm_word), &program)){
        printf("bad program\n");
        return 1;
    }

    srsvm_thread *main_thread = srsvm_vm_alloc_thread(vm, progdata->literal_start);

    if(main_thread == NULL){
        printf("bad thread\n");
        return 1;
    }

    srsvm_vm_start_thread(vm, main_thread->id);

    srsvm_vm_join_thread(vm, main_thread->id);
    
    srsvm_vm_free(vm);

    srsvm_virtual_memory_layout_free(list);
}
