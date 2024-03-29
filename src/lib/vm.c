#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "srsvm/constant.h"
#include "srsvm/debug.h"
#include "srsvm/mmu.h"
#include "srsvm/vm.h"

void mod_tree_free(const char* key, void* value, void* arg)
{
    if(value != NULL){
        srsvm_module_free(value);
    }
}

void srsvm_vm_free(srsvm_vm *vm)
{
    if(vm != NULL){

        if(vm->register_map != NULL){
            srsvm_string_map_free(vm->register_map, false);
        }
        if(vm->opcode_map != NULL) srsvm_opcode_map_free(vm->opcode_map);
        if(vm->module_map != NULL)
        {
            srsvm_string_map_walk(vm->module_map, mod_tree_free, NULL);
            srsvm_string_map_free(vm->module_map, false);
        }
        if(vm->mem_root != NULL){
            srsvm_mmu_set_all_free(vm->mem_root);
            srsvm_mmu_free(vm->mem_root);
        }

        for(int i = 0; i < SRSVM_REGISTER_MAX_COUNT; i++){
            if(vm->registers[i] != NULL){
                srsvm_register_free(vm->registers[i]);
            }
        }

        for(int i = 0; i < SRSVM_THREAD_MAX_COUNT; i++){
            if(vm->threads[i] != NULL){
                srsvm_thread_free(vm, vm->threads[i]);
            }
        }

        for(int i = 0; i < SRSVM_MODULE_MAX_COUNT; i++){
            if(vm->modules[i] != NULL){
                //srsvm_module_free(vm->modules[i]);
            }
        }

        for(int i = 0; i < SRSVM_CONST_MAX_COUNT; i++){
            if(vm->constants[i] != NULL){
                srsvm_const_free(vm->constants[i]);
            }
        }

        if(vm->module_search_path != NULL){
            for(size_t i = 0; vm->module_search_path[i] != NULL; i++){
                free(vm->module_search_path[i]);
            }

            free(vm->module_search_path);
        }

        free(vm);
    }
}

srsvm_vm *srsvm_vm_alloc(void)
{
    srsvm_vm *vm = NULL;

    vm = malloc(sizeof(srsvm_vm));

    if(vm != NULL){
        memset(vm->registers, 0, sizeof(vm->registers));
        memset(vm->threads, 0, sizeof(vm->threads));
        memset(vm->modules, 0, sizeof(vm->modules));
        memset(vm->constants, 0, sizeof(vm->constants));

        vm->main_thread = NULL;

        vm->mem_root = NULL;

        vm->has_program_loaded = false;

        vm->has_fault = false;
        memset(vm->fault_str, 0, sizeof(vm->fault_str));

        vm->argv = NULL;
        vm->argv = 0;

        srsvm_vm_set_module_search_path(vm, NULL);

        if((vm->opcode_map = srsvm_opcode_map_alloc()) == NULL){
            goto error_cleanup;
        } else if((vm->module_map = srsvm_string_map_alloc(true)) == NULL){
            goto error_cleanup;
        } else if((vm->register_map = srsvm_string_map_alloc(false)) == NULL){
            goto error_cleanup;
        } else if((vm->mem_root = srsvm_mmu_alloc_virtual(NULL, SRSVM_MAX_PTR, 0)) == NULL){
            goto error_cleanup;
        } else if(! load_builtin_opcodes(vm->opcode_map)){
            goto error_cleanup;
        }
    }

    return vm;

error_cleanup:
    srsvm_vm_free(vm);

    return NULL;
}

bool srsvm_vm_execute_instruction(srsvm_vm *vm, srsvm_thread *thread, const srsvm_instruction *instruction)
{
    bool success = false;

    srsvm_opcode *opcode = opcode_lookup_by_code(vm->opcode_map, instruction->opcode);

    if(opcode != NULL){
        if(strlen(opcode->name) > 0){
            dbg_printf("resolved opcode %s/" PRINT_WORD_HEX, opcode->name, PRINTF_WORD_PARAM(opcode->code));
        } else {
            dbg_printf("resolved opcode" PRINT_WORD_HEX, PRINTF_WORD_PARAM(opcode->code));
        }

        if(instruction->argc < opcode->argc_min || instruction->argc > opcode->argc_max){
            dbg_printf("invalid number of arguments " PRINT_WORD ", accepted range: [%hu,%hu]", PRINTF_WORD_PARAM(instruction->argc), opcode->argc_min, opcode->argc_max);

            snprintf(thread->fault_str, sizeof(thread->fault_str), "Illegal instruction length");
            thread->has_fault = true;
        } else {
            dbg_puts("executing opcode...");

            opcode->func(vm, thread, instruction->argc, instruction->argv);

            success = true;
        }
    } else {
        dbg_printf("failed to locate opcode " PRINT_WORD_HEX, PRINTF_WORD_PARAM(instruction->opcode));

        snprintf(thread->fault_str, sizeof(thread->fault_str), "Illegal instruction");
        thread->has_fault = true;
    }

    return success;
}

srsvm_thread *srsvm_vm_alloc_thread(srsvm_vm *vm, const srsvm_ptr start_addr, srsvm_ptr start_arg)
{
    srsvm_thread *thread = NULL;

    for(srsvm_word i = 0; i < SRSVM_THREAD_MAX_COUNT; i++){
        if(vm->threads[i] == NULL){
            thread = srsvm_thread_alloc(vm, i, start_addr, start_arg);

            vm->threads[i] = thread;

            break;
        }
    }

    return thread;
}

void srsvm_vm_thread_exit(srsvm_vm *vm, srsvm_thread *thread, srsvm_thread_exit_info *info)
{
    if(vm != NULL && thread != NULL){
        srsvm_thread_free(vm, thread);
    }

    srsvm_thread_exit(info);
}

typedef struct
{
    srsvm_vm *vm;
    srsvm_thread *thread;
} srsvm_thread_info;

void run_thread(void* arg)
{
    srsvm_thread_info *info = arg;

    while(! info->thread->is_halted && !info->vm->has_fault && !info->thread->has_fault){
        for(srsvm_word i = 0; i < SRSVM_REGISTER_MAX_COUNT; i++){
            if(info->vm->registers[i] != NULL){
                if(info->vm->registers[i]->fault_on_error && info->vm->registers[i]->error_flag){
                    strncpy(info->vm->fault_str, info->vm->registers[i]->error_str, sizeof(info->vm->fault_str));
                    info->vm->has_fault = true;
                }
            } else break;
        }

        if(! info->vm->has_fault){
            info->thread->PC = info->thread->next_PC;

            srsvm_instruction current_instruction;

            if(! srsvm_opcode_load_instruction(info->vm, info->thread->PC, &current_instruction)){
                info->thread->has_fault = true;
                snprintf(info->thread->fault_str, sizeof(info->thread->fault_str), "Failed to load instruction at address " PRINT_WORD, PRINTF_WORD_PARAM(info->thread->PC));
            } else {
                info->thread->next_PC = info->thread->PC + sizeof(current_instruction.opcode) + sizeof(srsvm_arg) * current_instruction.argc;

                dbg_printf("opcode: " PRINT_WORD_HEX, PRINTF_WORD_PARAM(current_instruction.opcode));
                dbg_printf("argc: " PRINT_WORD, PRINTF_WORD_PARAM(current_instruction.argc));
                for(srsvm_word i = 0; i < current_instruction.argc; i++){
                    dbg_printf("argv[" PRINT_WORD "]: { type: %u, value: " PRINT_WORD " }", PRINTF_WORD_PARAM(i), current_instruction.argv[i].type, PRINTF_WORD_PARAM(current_instruction.argv[i].value));
                }

                srsvm_vm_execute_instruction(info->vm, info->thread, &current_instruction);
            }

        }

        if(info->thread->has_fault && info->thread->fault_handler != NULL){
            info->thread->fault_handler(info->vm, info->thread);
        }

        if(info->vm->has_fault && info->vm->fault_handler != NULL){
            info->vm->fault_handler(info->vm);
        }
    }

    srsvm_thread_exit_info *exit_info = malloc(sizeof(srsvm_thread_exit_info));
    
    exit_info->ret = SRSVM_NULL_PTR;
    exit_info->has_fault = false;
    exit_info->fault_str = NULL;

    if(info->vm->has_fault){
        exit_info->has_fault = true;
        exit_info->fault_str = info->vm->fault_str;
    } else if(info->thread->has_fault){
        exit_info->has_fault = true;
        exit_info->fault_str = info->thread->fault_str;
    }

    srsvm_thread_exit(exit_info);
}

bool srsvm_vm_start_thread(srsvm_vm *vm, const srsvm_word thread_id)
{
    bool success = false;

    srsvm_thread_info *info = malloc(sizeof(srsvm_thread_info));

    if(info != NULL){
        info->vm = vm;
        info->thread = vm->threads[thread_id];

        if(thread_id < SRSVM_THREAD_MAX_COUNT && vm->threads[thread_id] != NULL){
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

    if(thread_id < SRSVM_THREAD_MAX_COUNT && vm->threads[thread_id] != NULL){
        if(srsvm_thread_join(vm->threads[thread_id], NULL)){
            success = true;
        }
    }

    return success;
}

void srsvm_vm_set_module_search_path(srsvm_vm *vm, const char* search_path)
{
    if(vm->module_search_path != NULL){
        for(size_t i = 0; vm->module_search_path[i] != NULL; i++){
            free(vm->module_search_path[i]);
        }

        free(vm->module_search_path);
        vm->module_search_path = NULL;
    }

    if(search_path != NULL){
        unsigned num_toks = 1;
        size_t arg_len = strlen(search_path);

        for(size_t i = 0; i < arg_len; i++){
            if(search_path[i] == ';'){
                num_toks++;
            }
        }

        char *writable_arg = srsvm_strdup(search_path);
        if(writable_arg != NULL){
            char **path = malloc((num_toks + 1) * sizeof(char*));
            for(int i = 0; i < num_toks+1; i++){
                path[i] = NULL;
            }

            if(path != NULL){
                size_t tok_num = 0;
                char *tok = strtok(writable_arg, ";");

                if(tok == NULL){
                    path[0] = srsvm_strdup(search_path);
                } else {
                    while(tok != NULL){
                        if(strlen(tok) == 0){
                            continue;
                        } else {
                            path[tok_num++] = srsvm_strdup(tok);
                        }

                        tok = strtok(NULL, ";");
                    }
                }

            }

            vm->module_search_path = path;

            free(writable_arg);
        } else {
            vm->module_search_path = NULL;
        }
    } else {
        srsvm_vm_set_module_search_path(vm, "mod");
    }

    if(vm->module_search_path == NULL){
        dbg_puts("cleared module search path");
    } else {
        dbg_puts("set module search path:");

        for(size_t i = 0; vm->module_search_path[i] != NULL; i++){
            dbg_printf(" [%lu] %s", i, vm->module_search_path[i]);
        }
    }

}

srsvm_module *srsvm_vm_load_module(srsvm_vm *vm, const char* module_name)
{
    srsvm_module *mod = NULL;

    char* file_path = NULL;
    char* prog_cwd = NULL;

    if((mod = srsvm_string_map_lookup(vm->module_map, module_name)) != NULL){
        mod->ref_count++;
    } else {
        prog_cwd = srsvm_getcwd();

        bool search_multilib = true;

search_again:
        file_path = srsvm_module_find(module_name, prog_cwd, vm->module_search_path, search_multilib);

        if(file_path != NULL){
            bool found_slot = false;
            srsvm_word mod_id;
            for(int i = 0; i < SRSVM_MODULE_MAX_COUNT; i++){
                if(vm->modules[i] == NULL){
                    mod_id = i;
                    found_slot = true;
                    break;
                }
            }

            if(found_slot){
                mod = srsvm_module_alloc(module_name, file_path, mod_id);

                if(mod == NULL || !srsvm_string_map_insert(vm->module_map, mod->name, mod)){
                    if(search_multilib){
                        if(mod != NULL){
                            srsvm_module_free(mod);
                            mod = NULL;
                        }
                        search_multilib = false;
                        goto search_again;
                    } else goto error_cleanup;
                } else {
                    vm->modules[mod_id] = mod;
                }
            } else {
                goto error_cleanup;
            }
        } else {
            goto error_cleanup;
        }
    }

    return mod;

error_cleanup:
    if(prog_cwd != NULL){
        free(prog_cwd);
    }

    if(file_path != NULL){
        free(file_path);
    }
    if(mod != NULL){
        srsvm_module_free(mod);
    }

    return NULL;
}

srsvm_module *srsvm_vm_load_module_slot(srsvm_vm *vm, const char* module_name, const srsvm_word slot_num)
{
    srsvm_module *mod = NULL;

    char* file_path = NULL;
    char* prog_cwd = NULL;

    if(slot_num < SRSVM_MODULE_MAX_COUNT){
        if(vm->modules[slot_num] != NULL){
            mod = srsvm_string_map_lookup(vm->module_map, module_name);

            if(mod != NULL){
                if(mod->id ==  slot_num){
                    return mod;
                } else {
                    return NULL;
                }
            } else {
                return NULL;
            }
        } else if((mod = srsvm_string_map_lookup(vm->module_map, module_name)) != NULL){
            mod->ref_count++;
        } else {
            prog_cwd = srsvm_getcwd();

            bool search_multilib = true;

search_again:
            file_path = srsvm_module_find(module_name, prog_cwd, vm->module_search_path, search_multilib);

            if(file_path != NULL){
                bool found_slot = false;
                srsvm_word mod_id;
                for(int i = 0; i < SRSVM_MODULE_MAX_COUNT; i++){
                    if(vm->modules[i] == NULL){
                        mod_id = i;
                        found_slot = true;
                        break;
                    }
                }

                if(found_slot){
                    mod = srsvm_module_alloc(module_name, file_path, mod_id);

                    if(mod == NULL || !srsvm_string_map_insert(vm->module_map, mod->name, mod)){
                        if(search_multilib){
                            if(mod != NULL){
                                srsvm_module_free(mod);
                            }
                            search_multilib = false;
                            goto search_again;
                        } else goto error_cleanup;
                    } else {
                        vm->modules[mod_id] = mod;
                    }
                } else {
                    goto error_cleanup;
                }
            } else {
                goto error_cleanup;
            }
        }
    }

    return mod;

error_cleanup:
    if(prog_cwd != NULL){
        free(prog_cwd);
    }

    if(file_path != NULL){
        free(file_path);
    }
    if(mod != NULL){
        srsvm_module_free(mod);
    }

    return NULL;
}

void srsvm_vm_unload_module(srsvm_vm *vm, srsvm_module *mod)
{
    mod->ref_count--;

    if(mod->ref_count == 0){
        vm->modules[mod->id] = NULL;

        srsvm_string_map_remove(vm->module_map, mod->name, false);

        srsvm_module_free(mod);
    }
}

srsvm_opcode *srsvm_vm_load_module_opcode(srsvm_vm *vm, srsvm_module *mod, const srsvm_word opcode)
{
    srsvm_opcode *op = NULL;

    if(mod != NULL){
        dbg_printf("attempting to load opcode " PRINT_WORD_HEX " from module %s", PRINTF_WORD_PARAM(opcode), mod->name);

        op = opcode_lookup_by_code(mod->opcode_map, opcode);
    }

    return op;
}

srsvm_register *srsvm_vm_register_alloc(srsvm_vm *vm, const char* name, const srsvm_word index)
{
    srsvm_register *reg = NULL;

    if(vm->registers[index] == NULL){
        reg = (vm->registers[index] = srsvm_register_alloc(name, index));
        srsvm_string_map_insert(vm->register_map, name, reg);
    }

    return reg;
}

static bool const_slot_in_use(const srsvm_vm *vm, const srsvm_word slot_num)
{
    return slot_num < SRSVM_CONST_MAX_COUNT && vm->constants[slot_num] != NULL;
}

#define CONST_ALLOCATOR(type,name,flag) \
    srsvm_constant_value *srsvm_vm_alloc_const_##name(srsvm_vm *vm, const srsvm_word index, const type value)\
{ \
    srsvm_constant_value *c = NULL; \
    if(index < SRSVM_CONST_MAX_COUNT && !const_slot_in_use(vm, index)){ \
        c = srsvm_const_alloc(SRSVM_TYPE_##flag); \
        if(c != NULL){ \
            vm->constants[index] = c; \
            c->name = value; \
        } \
    } \
    return c; \
}

CONST_ALLOCATOR(srsvm_word, word, WORD);
CONST_ALLOCATOR(srsvm_ptr, ptr, WORD);
CONST_ALLOCATOR(srsvm_ptr_offset, ptr_offset, PTR_OFFSET);
CONST_ALLOCATOR(bool, bit, BIT);
CONST_ALLOCATOR(uint8_t, u8, U8);
CONST_ALLOCATOR(int8_t, i8, I8);
CONST_ALLOCATOR(uint16_t, u16, U16);
CONST_ALLOCATOR(int16_t, i16, I16);
#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
CONST_ALLOCATOR(uint32_t, u32, U32);
CONST_ALLOCATOR(int32_t, i32, I32);
CONST_ALLOCATOR(float, f32, F32);
#endif
#if WORD_SIZE == 64 || WORD_SIZE == 128
CONST_ALLOCATOR(uint64_t, u64, U64);
CONST_ALLOCATOR(int64_t, i64, I64);
CONST_ALLOCATOR(double, f64, F64);
#endif
#if WORD_SIZE == 128
CONST_ALLOCATOR(unsigned __int128, u128, U128);
CONST_ALLOCATOR(__int128, i128, I128);
#endif
#undef CONST_ALLOCATOR

srsvm_constant_value *srsvm_vm_alloc_const_str(srsvm_vm *vm, const srsvm_word index, const char* value)\
{
    srsvm_constant_value *c = NULL;
    if(index < SRSVM_CONST_MAX_COUNT && !const_slot_in_use(vm, index)){
        c = srsvm_const_alloc(SRSVM_TYPE_STR);
        if(c != NULL){
            vm->constants[index] = c;
            c->str = value;
            c->str_len = strlen(value);
        }
    }
    return c;
}

bool srsvm_vm_load_const(srsvm_vm *vm, srsvm_register *dest_reg, const srsvm_word index, const srsvm_word offset)
{
    bool success = false;

    if(index < SRSVM_CONST_MAX_COUNT && const_slot_in_use(vm, index)){
        srsvm_constant_value *c = vm->constants[index];

        success = srsvm_const_load(dest_reg, c, offset);
    }

    return success;
}

bool srsvm_vm_load_program(srsvm_vm *vm, const srsvm_program *program)
{
    bool success = false;

    if(vm != NULL && program != NULL){
        if(vm->has_program_loaded){
            dbg_puts("ERRROR: attempted to load a program in to a VM which already has one loaded");
        } else {
            srsvm_register_specification *reg = program->registers;

            for(int i = 0; reg != NULL && i < program->num_registers; i++){
                if(! srsvm_vm_register_alloc(vm, reg->name, reg->index)){
                    return false;
                } else reg = reg->next;
            }

            srsvm_virtual_memory_specification *vmem = program->virtual_memory;

            for(int i = 0; vmem != NULL && i < program->num_vmem_segments; i++){
                if(srsvm_mmu_alloc_virtual(vm->mem_root, vmem->size, vmem->start_address) == NULL){
                    return false;
                } else vmem = vmem->next;
            }

            srsvm_literal_memory_specification *lmem = program->literal_memory;

            for(int i = 0; lmem != NULL && i < program->num_lmem_segments; i++){
                srsvm_memory_segment *lmem_seg = srsvm_mmu_alloc_literal(vm->mem_root, lmem->size, lmem->start_address);

                if(lmem_seg == NULL){
                    return false;
                } else if(! srsvm_mmu_store(vm->mem_root, lmem->start_address, lmem->size, lmem->data)){
                    return false;
                } else {
                    lmem_seg->readable = lmem->readable;
                    lmem_seg->writable = lmem->writable;
                    lmem_seg->executable = lmem->executable;

                    lmem_seg->locked = lmem->locked;

                    lmem = lmem->next;
                }
            }

            srsvm_constant_specification *c = program->constants;

            for(int i = 0; c != NULL && i < program->num_constants; i++){
                srsvm_constant_value *cv = &c->const_val;

                switch(cv->type){
#define LOADER(field, type) \
                    case SRSVM_TYPE_##type: \
                                            if(srsvm_vm_alloc_const_##field(vm, c->const_slot, cv->field) == NULL) { \
                                                return false; \
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
                    return false;
                }

                c = c->next;
            }

            srsvm_thread *thread = srsvm_vm_alloc_thread(vm, program->metadata->entry_point, 0);

            if(thread == NULL){
                return false;
            } else {
                vm->main_thread = thread;
            }

            success = true;
        }
    }

    return success;
}

void srsvm_vm_set_argv(srsvm_vm *vm, const char** argv, const int argc)
{
    vm->argv = argv;
    vm->argc = argc;
}

void srsvm_vm_set_fault_handler(srsvm_vm *vm, srsvm_vm_fault_handler fault_handler)
{
    vm->fault_handler = fault_handler;
}
