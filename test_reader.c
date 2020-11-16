#include <stdio.h>
#include <string.h>

#include "srsvm/debug.h"
#include "srsvm/mmu.h"
#include "srsvm/vm.h"

bool srsvm_debug_mode;

int main(){
    srsvm_debug_mode = false;

    srsvm_vm *vm = srsvm_vm_alloc();
    
    srsvm_program *program = srsvm_program_deserialize("hello_world.svm");

    if(program == NULL){
        printf("failed to deserialize program\n");
    }

    if(! srsvm_vm_load_program(vm, program)){
        printf("Failed to load program\n");
        srsvm_program_free(program);
        srsvm_vm_free(vm);
        return 1;
    }

    srsvm_thread *main_thread = vm->main_thread;

    srsvm_vm_start_thread(vm, main_thread->id);

    srsvm_vm_join_thread(vm, main_thread->id);
    
    srsvm_vm_free(vm);
    
    srsvm_program_free(program);
}
