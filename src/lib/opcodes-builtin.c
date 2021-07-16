#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "srsvm/debug.h"
#include "srsvm/mmu.h"
#include "srsvm/opcode-helpers.h"
#include "srsvm/module.h"
#include "srsvm/value_types.h"
#include "srsvm/vm.h"

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
    srsvm_register *dest_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *size_reg = register_lookup(vm, thread, argv[1]);

    if(dest_reg != NULL && size_reg != NULL && ! fault_on_not_writable(thread, dest_reg)){
        srsvm_memory_segment *seg = srsvm_mmu_alloc_literal(vm->mem_root, size_reg->value.word, 0);

        if(seg != NULL){
            if(! load_ptr(dest_reg, seg->literal_start, 0)){
                set_register_error_bit(dest_reg, "Failed to copy address to register");
                srsvm_mmu_free(seg);
            }
        } else {
            set_register_error_bit(dest_reg, "Failed to allocate memory");
        }
    }
}

void builtin_FREE(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *addr_reg = register_lookup(vm, thread, argv[0]);

    if(addr_reg != NULL){
        srsvm_memory_segment *seg = srsvm_mmu_locate(vm->mem_root, addr_reg->value.ptr);

        if(seg != NULL){
            srsvm_mmu_free(seg);
        } else {
            thread->has_fault = true;
            thread->fault_str = "Attempt to free an unallocated address";
        }
    }
}

void builtin_LOAD_CONST(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *dest_reg = register_lookup(vm, thread, argv[0]);
    srsvm_word const_index = argv[1];
    srsvm_word offset = 0;
    if(argc == 3){
        offset = argv[2];
    }

    if(dest_reg != NULL && !fault_on_not_writable(thread, dest_reg)){
        if(! srsvm_vm_load_const(vm, dest_reg, const_index, offset)){
            thread->has_fault = true;
            thread->fault_str = "Attmept to load an invalid or unallocated constant";
        }
    }
}

void builtin_LOAD(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *dest_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *src_addr_reg = register_lookup(vm, thread, argv[1]);

    srsvm_ptr src_addr = SRSVM_NULL_PTR;
    if(src_addr_reg != NULL){
        src_addr = src_addr_reg->value.ptr;
    }

    srsvm_value_type type = argv[2];
    srsvm_word offset = 0;
    if(argc == 4){
        offset = argv[3];
    }

    if(dest_reg != NULL && !fault_on_not_writable(thread, dest_reg)){
        if(type > SRSVM_MAX_TYPE_VALUE){
            thread->has_fault = true;
            thread->fault_str = "Attempt to load an invalid type";
        } else {
            switch(type){

            #define LOADER(name,flag) \
                case SRSVM_TYPE_##flag: \
                if(! mem_load_##name(vm, dest_reg, src_addr, offset)){ \
                    thread->has_fault = true; \
                    thread->fault_str = "Failed to load from memory into register"; \
                }  \
                break;

            LOADER(word, WORD);
            LOADER(ptr, PTR);
            LOADER(ptr_offset, PTR_OFFSET);
            LOADER(bit, BIT);
            LOADER(u8, U8);
            LOADER(i8, I8);
            LOADER(u16, U16);
            LOADER(i16, I16);
#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
            LOADER(u32, U32);
            LOADER(i32, I32);
            LOADER(f32, F32);
#endif
#if WORD_SIZE == 64 || WORD_SIZE == 128
            LOADER(u64, U64);
            LOADER(i64, I64);
            LOADER(f64, F64);
#endif
#if WORD_SIZE == 128
            LOADER(u128, U128);
            LOADER(i128, I128);
#endif
            LOADER(str, STR);
#undef LOADER
            default:
                thread->has_fault = true;
                thread->fault_str = "Attempt to load invalid type";
                break;
            }
        }
    }
}

void builtin_STORE(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *dest_addr_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *src_reg = register_lookup(vm, thread, argv[1]);

    srsvm_ptr dest_addr = SRSVM_NULL_PTR;
    if(dest_addr_reg != NULL){
        dest_addr = dest_addr_reg->value.ptr;
    }

    srsvm_value_type type = argv[2];
    srsvm_word offset = 0;
    if(argc == 4){
        offset = argv[3];
    }

    if(dest_addr_reg != NULL && src_reg != NULL){
        switch(type){

#define LOADER(name,flag) \
            case SRSVM_TYPE_##flag: \
                       if(! mem_store_##name(vm, src_reg, dest_addr, offset)){ \
                           thread->has_fault = true; \
                           thread->fault_str = "Failed to store value from register into memory"; \
                       } \
            break;

            LOADER(word, WORD);
            LOADER(ptr, PTR);
            LOADER(ptr_offset, PTR_OFFSET);
            LOADER(bit, BIT);
            LOADER(u8, U8);
            LOADER(i8, I8);
            LOADER(u16, U16);
            LOADER(i16, I16);
#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
            LOADER(u32, U32);
            LOADER(i32, I32);
            LOADER(f32, F32);
#endif
#if WORD_SIZE == 64 || WORD_SIZE == 128
            LOADER(u64, U64);
            LOADER(i64, I64);
            LOADER(f64, F64);
#endif
#if WORD_SIZE == 128
            LOADER(u128, U128);
            LOADER(i128, I128);
#endif
            LOADER(str, STR);
#undef LOADER
            default:
            thread->has_fault = true;
            thread->fault_str = "Attempt to store invalid type";
            break;
        }

    }

}

void builtin_MOD_LOAD(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *dest_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *mod_name_reg = register_lookup(vm, thread, argv[1]);

    if(dest_reg != NULL && mod_name_reg != NULL && !fault_on_not_writable(thread, dest_reg)){
        srsvm_module *mod = srsvm_vm_load_module(vm, mod_name_reg->value.str);

        if(mod != NULL){
            if(! load_word(dest_reg, mod->id, 0)){
                set_register_error_bit(dest_reg, "Failed to copy module ID to register");
                srsvm_vm_unload_module(vm, mod);
            }
        } else {
            set_register_error_bit(dest_reg, "Failed to load module");
        }
    }
}

void builtin_CMOD_LOAD(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *dest_reg = register_lookup(vm, thread, argv[0]);
    srsvm_word dest_slot = argv[1];
    srsvm_register *mod_name_reg = register_lookup(vm, thread, argv[2]);

    if(mod_name_reg != NULL && dest_reg != NULL && !fault_on_not_writable(thread, dest_reg)){
        srsvm_module *mod = srsvm_vm_load_module_slot(vm, mod_name_reg->value.str, dest_slot);

        if(mod != NULL){
            if(! load_word(dest_reg, mod->id, 0)){
                set_register_error_bit(dest_reg, "Failed to copy module ID to register");
                srsvm_vm_unload_module(vm, mod);
            }
        } else {
            set_register_error_bit(dest_reg, "Failed to load module");
        }
    }
}

void builtin_MOD_UNLOAD(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *mod_id_reg = register_lookup(vm, thread, argv[0]);

    if(mod_id_reg != NULL){
        if(mod_id_reg->value.word > SRSVM_MODULE_MAX_COUNT){
            thread->has_fault = true;
            thread->fault_str = "Attempt to unload an invalid module ID";
        } else {
            srsvm_module *mod = vm->modules[mod_id_reg->value.word];

            if(mod != NULL){
                srsvm_vm_unload_module(vm, mod);
            }
        }
    }
}

void builtin_CMOD_UNLOAD(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_word mod_id = argv[0];

    if(mod_id > SRSVM_MODULE_MAX_COUNT){
        thread->has_fault = true;
        thread->fault_str = "Attempt to unload an invalid module ID";
    } else {
        srsvm_module *mod = vm->modules[mod_id];

        if(mod != NULL){
            srsvm_vm_unload_module(vm, mod);
        }
    }
}

void builtin_MOD_OP(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *mod_id_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *mod_opcode_reg = register_lookup(vm, thread, argv[1]);

    srsvm_word shifted_argc = argc - 2;

    if(mod_id_reg != NULL && mod_opcode_reg != NULL){
        if(mod_id_reg->value.word > SRSVM_MODULE_MAX_COUNT){
            thread->has_fault = true;
            thread->fault_str = "Attempt to call an invalid module ID";
        } else {
            srsvm_module *mod = vm->modules[mod_id_reg->value.word];

            if(mod != NULL){
                srsvm_opcode *opcode = srsvm_vm_load_module_opcode(vm, mod, mod_opcode_reg->value.word);

                if(opcode != NULL){
                    if(shifted_argc < opcode->argc_min || shifted_argc > opcode->argc_max){
                        thread->has_fault = true;
                        thread->fault_str = "Wrong number of arguments for module opcode";
                    } else {
                        dbg_puts("calling mod func");

                        opcode->func(vm, thread, shifted_argc, argv + 2);
                    }
                } else {
                    thread->has_fault = true;
                    thread->fault_str = "Attempt to run an invalid opcode from a module";
                }
            } else {
                thread->has_fault = true;
                thread->fault_str = "Attempt to run an opcode from an unloaded module";
            }
        }
    }
}

void builtin_CMOD_OP(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_word mod_id = argv[0];
    srsvm_word mod_opcode = argv[1];

    srsvm_word shifted_argc = argc - 2;

    if(mod_id > SRSVM_MODULE_MAX_COUNT){
        thread->has_fault = true;
        thread->fault_str = "Attempt to unload an invalid module ID";
    } else {
        srsvm_module *mod = vm->modules[mod_id];

        if(mod != NULL){
            srsvm_opcode *opcode = srsvm_vm_load_module_opcode(vm, mod, mod_opcode);

            if(opcode != NULL){
                if(shifted_argc < opcode->argc_min || shifted_argc > opcode->argc_max){
                    thread->has_fault = true;
                    thread->fault_str = "Wrong number of arguments for module opcode";
                } else {
                    opcode->func(vm, thread, shifted_argc, argv + 2);
                }
            } else {
                thread->has_fault = true;
                thread->fault_str = "Attempt to run an invalid opcode from a module";
            }
        } else {
            thread->has_fault = true;
            thread->fault_str = "Attempt to run an opcode from an unloaded module";
        }
    }
}

void builtin_MOD_UNLOAD_ALL(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    for(srsvm_word mod_id = 0; mod_id < SRSVM_MODULE_MAX_COUNT; mod_id++){
        srsvm_module *mod = vm->modules[mod_id];

        if(mod != NULL){
            srsvm_vm_unload_module(vm, mod);
        }
    }
}

void builtin_JMP(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *target_addr_reg = register_lookup(vm, thread, argv[0]);

    if(target_addr_reg != NULL){
        thread->next_PC = target_addr_reg->value.ptr;
    }
}

void builtin_JMP_OFF(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *target_off_reg = register_lookup(vm, thread, argv[0]);

    if(target_off_reg != NULL){
        thread->next_PC = thread->PC + target_off_reg->value.ptr_offset;
    }
}

void builtin_JMP_IF(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *target_addr_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *condition_reg = register_lookup(vm, thread, argv[1]);

    if(target_addr_reg != NULL && condition_reg != NULL){
        if(condition_reg->value.bit){
            thread->next_PC = target_addr_reg->value.ptr;
        }
    }
}

void builtin_JMP_OFF_IF(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *target_off_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *condition_reg = register_lookup(vm, thread, argv[1]);

    if(target_off_reg != NULL && condition_reg != NULL){
        if(condition_reg->value.bit){
            thread->next_PC = thread->PC + target_off_reg->value.ptr_offset;
        }
    }
}

void builtin_JMP_ERR(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *target_addr_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *condition_reg = register_lookup(vm, thread, argv[1]);

    if(target_addr_reg != NULL && condition_reg != NULL){
        if(condition_reg->error_flag){
            thread->next_PC = target_addr_reg->value.ptr;
        }
    }
}

void builtin_JMP_OFF_ERR(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *target_off_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *condition_reg = register_lookup(vm, thread, argv[1]);

    if(target_off_reg != NULL && condition_reg != NULL){
        if(condition_reg->error_flag){
            thread->next_PC = thread->PC + target_off_reg->value.ptr_offset;
        }
    }
}

void builtin_PUTS(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *src_str_reg = register_lookup(vm, thread, argv[0]);

    if(src_str_reg != NULL){
        puts(src_str_reg->value.str);
    }
}

void builtin_INCR(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *reg = register_lookup(vm, thread, argv[0]);

    if(reg != NULL && !fault_on_not_writable(thread, reg)){
        srsvm_word val = reg->value.word + 1;

        if(! load_word(reg, val, 0)){
            dbg_puts("failed to load word into register");
        }
    }
}

void builtin_DECR(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *reg = register_lookup(vm, thread, argv[0]);

    if(reg != NULL && !fault_on_not_writable(thread, reg)){
        srsvm_word val = reg->value.word - 1;

        if(! load_word(reg, val, 0)){
            dbg_puts("failed to load word into register");
        }
    }
}

void builtin_WORD_EQ(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *dest_reg = register_lookup(vm, thread, argv[0]);
    srsvm_register *a_reg = register_lookup(vm, thread, argv[1]);
    srsvm_register *b_reg = register_lookup(vm, thread, argv[2]);

    if(dest_reg != NULL && a_reg != NULL && b_reg != NULL && !fault_on_not_writable(thread, dest_reg)){
        srsvm_word a = a_reg->value.word;
        srsvm_word b = b_reg->value.word;

        load_bit(dest_reg, a == b, 0);
    }
}

void builtin_SLEEP(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_register *duration_reg = register_lookup(vm, thread, argv[0]);

    if(duration_reg != NULL){
        srsvm_sleep(duration_reg->value.word);
    }
}

void builtin_CJMP_BACK(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_ptr_offset offset = -1 * argv[0];

    if(offset != 0){
        thread->next_PC = thread->PC + offset;
    }
}

void builtin_CJMP_BACK_IF(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_ptr_offset offset = -1 * argv[0];
    srsvm_register *cond_reg = register_lookup(vm, thread, argv[1]);

    if(cond_reg != NULL && cond_reg->value.bit){
        if(offset != 0){
            thread->next_PC = thread->PC + offset;
        }
    }
}

void builtin_CJMP_BACK_ERR(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_ptr_offset offset = -1 * argv[0];
    srsvm_register *cond_reg = register_lookup(vm, thread, argv[1]);

    if(cond_reg != NULL && cond_reg->error_flag){
        if(offset != 0){
            thread->next_PC = thread->PC + offset;
        }
    }
}

void builtin_CJMP_FORWARD(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_ptr_offset offset = argv[0];

    if(offset != 0){
        thread->next_PC = thread->PC + offset;
    }
}

void builtin_CJMP_FORWARD_IF(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_ptr_offset offset = argv[0];
    srsvm_register *cond_reg = register_lookup(vm, thread, argv[1]);

    if(cond_reg != NULL && cond_reg->value.bit){
        if(offset != 0){
            thread->next_PC = thread->PC + offset;
        }
    }
}

void builtin_CJMP_FORWARD_ERR(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[])
{
    srsvm_ptr_offset offset = argv[0];
    srsvm_register *cond_reg = register_lookup(vm, thread, argv[1]);

    if(cond_reg != NULL && cond_reg->error_flag){
        if(offset != 0){
            thread->next_PC = thread->PC + offset;
        }
    }
}

static bool register_opcode(srsvm_opcode_map *map, srsvm_word code, const char* name, const unsigned short argc_min, const unsigned short argc_max, srsvm_opcode_func* func)
{
    bool success = false;

    if((code & OPCODE_ARGC_MASK) != 0){
        dbg_printf("Error: opcode " PRINT_WORD_HEX " & OPCODE_ARGC_MASK != 0; opcode will not be called.", PRINTF_WORD_PARAM(code));
    } else if(argc_min > MAX_INSTRUCTION_ARGS){
        dbg_printf("Error: opcode " PRINT_WORD_HEX " has min argc %hu, > %u", PRINTF_WORD_PARAM(code), argc_min, MAX_INSTRUCTION_ARGS);
    } else if(argc_max > MAX_INSTRUCTION_ARGS){
        dbg_printf("Error: opcode " PRINT_WORD_HEX " has max argc %hu, > %u", PRINTF_WORD_PARAM(code), argc_max, MAX_INSTRUCTION_ARGS);
    } else if(strlen(name) > 0 && opcode_lookup_by_name(map, name) != NULL){
        dbg_printf("Error: opcode " PRINT_WORD_HEX " has duplicate mnemonic %s", PRINTF_WORD_PARAM(code), name);
    } else if(opcode_lookup_by_code(map, code) != NULL){
        dbg_printf("Error: opcode " PRINT_WORD_HEX " has duplicate code number", PRINTF_WORD_PARAM(code));
    } else {
        srsvm_opcode *op;

        if((op = malloc(sizeof(srsvm_opcode))) == NULL){

        } else {

            op->code = code;
            memset(op->name, 0, sizeof(op->name));
            if(strlen(name) > 0){
				srsvm_strncpy(op->name, name, sizeof(op->name) - 1);
            }
            op->argc_min = argc_min;
            op->argc_max = argc_max;
            op->func = func;

            if(! opcode_map_insert(map, op)){

            } else {
                success = true;
            }
        }
    }

    return success;
}

bool load_builtin_opcodes(srsvm_opcode_map *map)
{
    bool success = true;

#define REGISTER_OPCODE(c,n,a_min,a_max) do { \
    if(! register_opcode(map,c,#n,a_min,a_max,&builtin_##n)) { success = false; } \
} while(0)

#include "srsvm/opcodes-builtin.h"

#undef REGISTER_OPCODE

    return success;
    }

