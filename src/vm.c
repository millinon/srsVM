#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "debug.h"
#include "mmu.h"
#include "vm.h"

void srsvm_vm_free(srsvm_vm *vm)
{
    // srsvm_opcode_map_free(vm->opcode_map);
    srsvm_mmu_free_force(vm->mem_root);
    
    for(srsvm_word i = 0; i < MAX_REGISTER_COUNT; i++){
        if(vm->registers[i] != NULL){
            // srsvm_register_free(vm->registers[i]);
        }
    }

    for(srsvm_word i = 0; i < MAX_THREAD_COUNT; i++){
        if(vm->threads[i] != NULL){
            srsvm_thread_free(vm, vm->threads[i]);
        }
    }

}

srsvm_vm *srsvm_vm_alloc(srsvm_virtual_memory_desc *memory_layout)
{
    srsvm_vm *vm = NULL;

    vm = malloc(sizeof(srsvm_vm));

    if(vm != NULL){
        if((vm->opcode_map = srsvm_opcode_map_alloc()) == NULL){
            free(vm);
            vm = NULL;
        } else if((vm->mem_root = srsvm_mmu_alloc_virtual(NULL, SRSVM_MAX_PTR, 0)) == NULL){
            free(vm->opcode_map);
            free(vm);
            vm = NULL;
        } else {
            while(memory_layout != NULL){
                if(srsvm_mmu_alloc_virtual(vm->mem_root, memory_layout->size, memory_layout->start_address) == NULL){
                    srsvm_mmu_free_force(vm->mem_root);
                    
                    free(vm->opcode_map);
                    free(vm);
                    vm = NULL;
                    break;
                } else {
                    memory_layout = memory_layout->next;
                }
            }


            if(vm != NULL){
                for(int i = 0; i < MAX_REGISTER_COUNT; i++){
                    vm->registers[i] = NULL;
                }

                for(int i = 0; i < MAX_THREAD_COUNT; i++){
                    vm->threads[i] = NULL;
                }
                
                if(! load_builtin_opcodes(vm)){
                    srsvm_mmu_free_force(vm->mem_root);
                    
                    free(vm->opcode_map);
                    free(vm);
                    vm = NULL;
                }
            }
        }
    }

    return vm;
}

bool srsvm_vm_execute_instruction(srsvm_vm *vm, srsvm_thread *thread, const srsvm_instruction *instruction)
{
    bool success = false;

    srsvm_opcode *opcode = opcode_lookup_by_code(vm->opcode_map, instruction->opcode);

    if(opcode != NULL){
        if(strlen(opcode->name) > 0){
            dbg_printf("resolved opcode %s/" SWFX, opcode->name, opcode->code);
        } else {
            dbg_printf("resolved opcode" SWFX, opcode->code);
        }

        if(instruction->argc < opcode->argc_min || instruction->argc > opcode->argc_max){
            dbg_printf("invalid number of arguments %hu, accepted range: [%hu,%hu]", instruction->argc, opcode->argc_min, opcode->argc_max);

            thread->fault_str = "Illegal instruction length";
            thread->has_fault = true;
        } else {
            dbg_puts("executing opcode...");

            opcode->func(vm, thread, instruction->argc, instruction->argv);

            success = true;
        }
    } else {
        dbg_printf("failed to locate opcode " SWFX, instruction->opcode);

        thread->fault_str = "Illegal instruction";
        thread->has_fault = true;
    }

    return success;
}

srsvm_thread *srsvm_vm_alloc_thread(srsvm_vm *vm, const srsvm_ptr start_addr)
{
    srsvm_thread *thread = NULL;

    for(srsvm_word i = 0; i < MAX_THREAD_COUNT; i++){
        if(vm->threads[i] == NULL){
            thread = srsvm_thread_alloc(vm, i, start_addr);
    
            vm->threads[i] = thread;

            break;
        }
    }

    return thread;
}

typedef struct
{
    srsvm_vm *vm;
    srsvm_thread *thread;
} srsvm_thread_info;

void run_thread(void* arg)
{
    srsvm_thread_info *info = arg;

    while(! info->thread->is_halted && ! info->thread->has_fault){
        srsvm_instruction current_instruction;

        if(! load_instruction(info->vm, info->thread->PC, &current_instruction)){
            info->thread->has_fault = true;
        } else {
            info->thread->PC = info->thread->PC + sizeof(current_instruction.opcode) + sizeof(srsvm_word) * current_instruction.argc;

            srsvm_vm_execute_instruction(info->vm, info->thread, &current_instruction);
        }
    }

    srsvm_thread_exit(info->thread);
}

bool srsvm_vm_start_thread(srsvm_vm *vm, const srsvm_word thread_id)
{
    bool success = false;

    srsvm_thread_info *info = malloc(sizeof(srsvm_thread_info));

    if(info != NULL){
        info->vm = vm;
        info->thread = vm->threads[thread_id];

        if(thread_id < MAX_THREAD_COUNT && vm->threads[thread_id] != NULL){
            if(srsvm_thread_start(vm->threads[thread_id], run_thread, info)){
                success = true;
            }
        }
    }

    return success;
}

bool srsvm_vm_join_thread(srsvm_vm *vm, const srsvm_word thread_id)
{
    bool success = false;

    if(thread_id < MAX_THREAD_COUNT && vm->threads[thread_id] != NULL){
        if(srsvm_thread_join(vm->threads[thread_id])){
            success = true;
        }
    }

    return success;
}
