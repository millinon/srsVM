#include <stdlib.h>
#include <string.h>

#include "srsvm/debug.h"
#include "srsvm/thread.h"
#include "srsvm/vm.h"

srsvm_thread *srsvm_thread_alloc(srsvm_vm *vm, srsvm_word id, srsvm_ptr start_addr)
{
    srsvm_thread *thread;

    thread = malloc(sizeof(srsvm_thread));

    if(thread != NULL){
        thread->id = id;
        thread->PC = SRSVM_NULL_PTR;
        thread->next_PC = start_addr;

        thread->is_halted = false;
        thread->has_fault = false;
        thread->fault_str = NULL;

        thread->fault_handler = NULL;

        srsvm_stack_frame *base_frame = malloc(sizeof(srsvm_stack_frame));
        if(base_frame != NULL){
            //base_frame->PC = start_addr;
            base_frame->next_PC = start_addr;
            base_frame->spilled = NULL;
            base_frame->last_spilled = NULL;
            base_frame->next = NULL;
        
            thread->call_stack.frames = base_frame;
            thread->call_stack.top = base_frame;
        } else {
            free(thread);
            thread = NULL;
        }
    }
    
    return thread;
}

void srsvm_thread_free(srsvm_vm *vm, srsvm_thread *thread)
{
    srsvm_stack_frame *frame = thread->call_stack.frames, *next_frame;

    while(frame != NULL){
        srsvm_spilled_register *spilled = frame->spilled, *next_spilled;

        while(spilled != NULL){
            next_spilled = spilled->next;
            free(spilled);
            spilled = next_spilled;
        }

        next_frame = frame->next;
        free(frame);
        frame = next_frame;
    }

    vm->threads[thread->id] = NULL;

    free(thread);
}

bool srsvm_push(srsvm_vm *vm, srsvm_thread *thread, const srsvm_register *reg)
{
    bool success = false;

    srsvm_stack_frame *cur_frame = thread->call_stack.top;

    if(cur_frame != NULL){
        srsvm_spilled_register *spill = malloc(sizeof(srsvm_spilled_register));

        if(spill != NULL){
            spill->index = reg->index;
            memcpy(&spill->reg, reg, sizeof(srsvm_register));
            spill->last = cur_frame->last_spilled;
            spill->next = NULL;

            if(cur_frame->spilled == NULL){
                cur_frame->spilled = spill;
            }

            cur_frame->last_spilled = spill;

            if(spill->last != NULL){
                spill->last->next = spill;
            }
        
            success = true;
        }
    }

    return success;
}

bool srsvm_pop(srsvm_vm *vm, srsvm_thread *thread)
{
    bool success = false;
    
    srsvm_stack_frame *cur_frame = thread->call_stack.top;

    if(cur_frame != NULL){
        srsvm_spilled_register *spilled = cur_frame->last_spilled;

        if(spilled != NULL){
            cur_frame->last_spilled = spilled->last;

            if(spilled->last != NULL){
                spilled->last->next = NULL;
            }

            if(cur_frame->spilled == spilled){
                cur_frame->spilled = NULL;
            }

            srsvm_register *reg = vm->registers[spilled->index];
            
            if(reg != NULL){
                if(reg->value.str != NULL && reg->value.str != spilled->reg.value.str){
                    free(reg->value.str);
                }

                memcpy(reg, &spilled->reg, sizeof(srsvm_register));

                free(spilled);

                success = true;
            }
        }
    }

    return success;
}

bool srsvm_call(srsvm_vm *vm, srsvm_thread *thread, const srsvm_ptr addr)
{   
    bool success = false;

    srsvm_stack_frame *last_frame = thread->call_stack.top;

    srsvm_stack_frame *new_frame = malloc(sizeof(srsvm_stack_frame));
    
    if(new_frame != NULL){
        new_frame->last = last_frame;
        new_frame->next = NULL;

        last_frame->next = new_frame;

        new_frame->next_PC = addr;

        new_frame->spilled = NULL;
        new_frame->last_spilled = NULL;

        thread->next_PC = addr;

        success = true;
    }

    return success;
}

bool srsvm_ret(srsvm_vm *vm, srsvm_thread *thread)
{
    bool success = false;

    srsvm_stack_frame *last_frame = thread->call_stack.top;
   
    if(last_frame == thread->call_stack.frames){

    } else {
        thread->call_stack.top = last_frame->last;
        thread->call_stack.top->next = NULL;

        thread->next_PC = thread->call_stack.top->next_PC;

        srsvm_spilled_register *next_reg;
        for(srsvm_spilled_register *reg = last_frame->spilled; reg != NULL; reg = next_reg){
            next_reg = reg->next;

            free(reg);
        }

        success = true;
    }

    return success;
}

void srsvm_thread_set_fault_handler_native(srsvm_thread *thread, srsvm_thread_fault_handler handler)
{
    if(thread != NULL){
        if(handler != NULL){
            dbg_printf("setting fault handler for thread %p to native function %p", thread, handler);
        } else {
            dbg_printf("clearing fault handler for thread %p", thread);
        }

        thread->fault_handler = handler;
        thread->fault_handler_addr = SRSVM_NULL_PTR;
    }
}

static void srsvm_hosted_fault_handler(srsvm_vm *vm, srsvm_thread *thread)
{
    srsvm_instruction instruction;
    
    srsvm_ptr fault_handler_addr = thread->fault_handler_addr;

    thread->fault_handler = NULL;
    thread->fault_handler_addr = SRSVM_NULL_PTR;

    if(srsvm_call(vm, thread, thread->fault_handler_addr)){
        srsvm_stack_frame *handler_frame = thread->call_stack.top;

        if(handler_frame->last != NULL){
            srsvm_stack_frame *prev_frame = handler_frame->last;

            while(! thread->is_halted && prev_frame->next == handler_frame){
                if(srsvm_opcode_load_instruction(vm, thread->PC, &instruction)){
                    thread->next_PC = thread->PC + sizeof(instruction.opcode) + sizeof(srsvm_word) * instruction.argc;

                    srsvm_vm_execute_instruction(vm, thread, &instruction);
                }
            }
        }
    }

    if(! thread->has_fault){
        thread->fault_handler = srsvm_hosted_fault_handler;
        thread->fault_handler_addr = fault_handler_addr;
    }
}

void srsvm_thread_set_fault_handler_hosted(srsvm_thread *thread, const srsvm_ptr handler_address)
{
    if(thread != NULL){
        dbg_printf("setting fault handler for thread %p to hosted address " PRINT_WORD_HEX, thread, PRINTF_WORD_PARAM(handler_address));

        thread->fault_handler = srsvm_hosted_fault_handler;
        thread->fault_handler_addr = handler_address;
    }
}
