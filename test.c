#include <stdio.h>

#include "debug.h"
#include "mmu.h"
#include "vm.h"

bool srsvm_debug_mode;

int main(){
    srsvm_debug_mode = true;

    srsvm_virtual_memory_desc *list = NULL;

    list = srsvm_virtual_memory_layout(NULL, 0x1000, 512 * WORD_SIZE);

    srsvm_vm *vm = srsvm_vm_alloc(list);
    
    if(vm == NULL){
        printf("Failed to allocate VM\n");
        return 1;
    }

    srsvm_vm_set_module_search_path(vm, "test;another/test;/yet/another/test");

    if(srsvm_vm_alloc_const_str(vm, 0, "Hello, world!") == NULL){
        printf("Failed to allocate string constant");
        return 1;
    }

    if(srsvm_vm_register_alloc(vm, "REG", 0) == NULL){
        printf("failed to allocate register");
        return 1;
    }

    srsvm_memory_segment *progdata = srsvm_mmu_alloc_literal(vm->mem_root, 512 * WORD_SIZE, 0x1000);

    if(progdata == NULL){
        printf("Failed to allocate program memory\n");
        return 1;
    }

    srsvm_word program[] = {
        (OPCODE_MK_ARGC(2) | 7), 0, 0, 
        (OPCODE_MK_ARGC(1) | 1337), 0,
        (OPCODE_MK_ARGC(0) | 1),
    };

    if(! srsvm_mmu_store(progdata, progdata->literal_start, sizeof(program), &program)){
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
