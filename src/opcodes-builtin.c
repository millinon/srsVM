#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mmu.h"
#include "opcode.h"
#include "opcode-helpers.h"
#include "vm.h"


void builtin_NOP(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{

}

void builtin_HALT(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    thread->is_halted = true;

    if(argc == 1){
        thread->exit_status = argv[0];
    } else {
        thread->exit_status = 0;
    }
}

void builtin_ALLOC(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{

}

void builtin_FREE(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{

}


static bool register_opcode(srsvm_vm *vm, srsvm_word code, const char* name, const unsigned short argc_min, const unsigned short argc_max, srsvm_opcode_func* func)
{
    bool success = false;

    if(strlen(name) > 0 && opcode_lookup_by_name(vm->opcode_map, name) != NULL){

    } else if(opcode_lookup_by_code(vm->opcode_map, code) != NULL){

    } else {
        srsvm_opcode *op;

        if((op = malloc(sizeof(srsvm_opcode))) == NULL){

        } else {

            op->code = code;
            memset(op->name, 0, sizeof(op->name));
            if(strlen(name) > 0){
                strncpy(op->name, name, sizeof(op->name));
            }
            op->argc_min = argc_min;
            op->argc_max = argc_max;
            op->func = func;

            if(! opcode_map_insert(vm->opcode_map, op)){

            } else {
                success = true;
            }
        }
    }
    
    return success;
}

bool load_builtin_opcodes(srsvm_vm *vm)
{
    bool success = true;

#define REGISTER_OPCODE(c,n,a_min,a_max) do { \
    if(! register_opcode(vm,c,#n,a_min,a_max,&builtin_##n)) { success = false; } \
} while(0)

#include "opcodes-builtin.h"

    return success;
}

