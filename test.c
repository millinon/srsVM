#include <stdio.h>

#include "debug.h"
#include "mmu.h"
#include "vm.h"

int main(){
    srsvm_virtual_memory_desc *list = NULL;

    list = srsvm_virtual_memory_layout(NULL, 0x1000, 512);

    srsvm_vm *vm = srsvm_vm_alloc(list);
    
    if(vm == NULL){
        printf("Failed to allocate VM\n");
        return 1;
    }

    srsvm_vm_set_module_search_path(vm, "test;another/test;/yet/another/test");

    srsvm_memory_segment *progdata = srsvm_mmu_alloc_literal(vm->mem_root, 16, 0x1000);

    if(progdata == NULL){
        printf("Failed to allocate program memory\n");
        return 1;
    }

    srsvm_word program[] = {
        0,
        1,
    };

    if(! srsvm_mmu_store(progdata, progdata->literal_start, 2 * sizeof(srsvm_word), &program)){
        printf("Failed to write program data to memory");
        return 1;
    }

    srsvm_thread *main_thread = srsvm_vm_alloc_thread(vm, progdata->literal_start);

    if(main_thread == NULL){
        printf("Failed to allocate thread");
        return 1;
    }

    srsvm_vm_start_thread(vm, main_thread->id);

    srsvm_vm_join_thread(vm, main_thread->id);
    
    srsvm_vm_free(vm);

    srsvm_virtual_memory_layout_free(list);
}
