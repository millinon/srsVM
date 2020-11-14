#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "constant.h"
#include "debug.h"
#include "mmu.h"
#include "vm.h"

void srsvm_vm_free(srsvm_vm *vm)
{
    if(vm != NULL){
        if(vm->opcode_map != NULL) srsvm_opcode_map_free(vm->opcode_map);
        if(vm->module_map != NULL) srsvm_module_map_free(vm->module_map);
        if(vm->mem_root != NULL) srsvm_mmu_free_force(vm->mem_root);

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
                srsvm_module_free(vm->modules[i]);
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

srsvm_vm *srsvm_vm_alloc(srsvm_virtual_memory_desc *memory_layout)
{
    srsvm_vm *vm = NULL;

    vm = malloc(sizeof(srsvm_vm));

    if(vm != NULL){
        for(int i = 0; i < SRSVM_REGISTER_MAX_COUNT; i++){
            vm->registers[i] = NULL;
        }

        for(int i = 0; i < SRSVM_THREAD_MAX_COUNT; i++){
            vm->threads[i] = NULL;
        }

        for(int i = 0; i < SRSVM_MODULE_MAX_COUNT; i++){
            vm->modules[i] = NULL;
        }

        for(int i = 0; i < SRSVM_CONST_MAX_COUNT; i++){
            vm->constants[i] = NULL;
        }

        srsvm_vm_set_module_search_path(vm, NULL);

        if((vm->opcode_map = srsvm_opcode_map_alloc()) == NULL){
            goto error_cleanup;
        } else if((vm->module_map = srsvm_module_map_alloc()) == NULL){
            goto error_cleanup;
        } else if((vm->mem_root = srsvm_mmu_alloc_virtual(NULL, SRSVM_MAX_PTR, 0)) == NULL){
            goto error_cleanup;
        } else {
            while(memory_layout != NULL){
                if(srsvm_mmu_alloc_virtual(vm->mem_root, memory_layout->size, memory_layout->start_address) == NULL){
                    goto error_cleanup;
                } else {
                    memory_layout = memory_layout->next;
                }
            }

            if(! load_builtin_opcodes(vm)){
                goto error_cleanup;
            }
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
            dbg_printf("resolved opcode %s/" SWFX, opcode->name, SW_PARAM(opcode->code));
        } else {
            dbg_printf("resolved opcode" SWFX, SW_PARAM(opcode->code));
        }

        if(instruction->argc < opcode->argc_min || instruction->argc > opcode->argc_max){
            dbg_printf("invalid number of arguments " SWF ", accepted range: [%hu,%hu]", SW_PARAM(instruction->argc), opcode->argc_min, opcode->argc_max);

            thread->fault_str = "Illegal instruction length";
            thread->has_fault = true;
        } else {
            dbg_puts("executing opcode...");

            opcode->func(vm, thread, instruction->argc, instruction->argv);

            success = true;
        }
    } else {
        dbg_printf("failed to locate opcode " SWFX, SW_PARAM(instruction->opcode));

        thread->fault_str = "Illegal instruction";
        thread->has_fault = true;
    }

    return success;
}

srsvm_thread *srsvm_vm_alloc_thread(srsvm_vm *vm, const srsvm_ptr start_addr)
{
    srsvm_thread *thread = NULL;

    for(srsvm_word i = 0; i < SRSVM_THREAD_MAX_COUNT; i++){
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

        if(! srsvm_opcode_load_instruction(info->vm, info->thread->PC, &current_instruction)){
            info->thread->has_fault = true;
        } else {
            info->thread->PC = info->thread->PC + sizeof(current_instruction.opcode) + sizeof(srsvm_word) * current_instruction.argc;

            srsvm_vm_execute_instruction(info->vm, info->thread, &current_instruction);
        }

        if(info->thread->has_fault && info->thread->fault_handler != NULL){
            info->thread->fault_handler(info->vm, info->thread);
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
        if(srsvm_thread_join(vm->threads[thread_id])){
            success = true;
        }
    }

    return success;
}

void srsvm_vm_set_module_search_path(srsvm_vm *vm, const char* search_path)
{
    if(search_path != NULL){
        unsigned num_toks = 1;
        size_t arg_len = strlen(search_path);

        for(size_t i = 0; i < arg_len; i++){
            if(search_path[i] == ';'){
                num_toks++;
            }
        }

        char *writable_arg = strdup(search_path);
        if(writable_arg != NULL){
            char **path = malloc((num_toks + 1) * sizeof(char*));
            for(int i = 0; i < num_toks+1; i++){
                path[i] = NULL;
            }

            if(path != NULL){
                size_t tok_num = 0;
                char *tok = strtok(writable_arg, ";");

                if(tok == NULL){
                    path[0] = strdup(search_path);
                } else {
                    while(tok != NULL){
                        if(strlen(tok) == 0){
                            continue;
                        } else {
                            path[tok_num++] = strdup(tok);
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
        vm->module_search_path = NULL;
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

static char* find_module(const char* module_name, char** search_path)
{
    char* module_path = NULL;

    char* mod_filename = srsvm_module_name_to_filename(module_name);

    if(mod_filename != NULL){
        if(search_path != NULL){
            for(size_t i = 0; search_path[i] != NULL; i++){
                if(srsvm_directory_exists(search_path[i])){
                    module_path = srsvm_path_combine(search_path[i], mod_filename);

                    if(! srsvm_file_exists(module_path)){
                        free(module_path);
                        module_path = NULL;
                    } else break;
                }
            }
        }

        if(module_path == NULL){
            char* cwd = srsvm_getcwd();

            if(cwd != NULL){
                module_path = srsvm_path_combine(cwd, mod_filename);

                if(! srsvm_file_exists(module_path)){
                    free(module_path);
                    module_path = NULL;
                }

                free(cwd);
            }
        }

        free(mod_filename);
    }

    return module_path;
}

srsvm_module *srsvm_vm_load_module(srsvm_vm *vm, const char* module_name)
{
    srsvm_module *mod = NULL;

    if((mod = srsvm_module_lookup(vm->module_map, module_name)) != NULL){
        mod->ref_count++;
    } else {
        const char* file_path = find_module(module_name, vm->module_search_path);

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

                if(mod != NULL || !srsvm_module_map_insert(vm->module_map, mod)){
                    goto error_cleanup;
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

        srsvm_module_map_remove(vm->module_map, mod->name);

        srsvm_module_free(mod);
    }
}

srsvm_opcode *srsvm_vm_load_module_opcode(srsvm_vm *vm, srsvm_module *mod, const srsvm_word opcode)
{
    srsvm_opcode *op = NULL;

    if(mod != NULL){
        dbg_printf("attempting to load opcode " SWFX " from module %s", SW_PARAM(opcode), mod->name);

        op = opcode_lookup_by_code(mod->opcode_map, opcode);
    }

    return op;
}

srsvm_register *srsvm_vm_register_alloc(srsvm_vm *vm, const char* name, const srsvm_word index)
{
    srsvm_register *reg = NULL;

    if(vm->registers[index] == NULL){
        reg = (vm->registers[index] = srsvm_register_alloc(name, index));
    }

    return reg;
}

srsvm_register *srsvm_vm_register_lookup(const srsvm_vm *vm, srsvm_thread *thread, const srsvm_word index)
{
    srsvm_register *reg = NULL;

   if(index < SRSVM_REGISTER_MAX_COUNT){
        reg = vm->registers[index];

        if(reg == NULL){
            thread->has_fault = true;
            thread->fault_str = "Unallocated register accessed";
        }
   } else {
        thread->has_fault = true;
        thread->fault_str = "Invalid register index specified";
   }

    return reg;
}

static bool const_slot_in_use(const srsvm_vm *vm, const srsvm_word slot_num)
{
    return slot_num < SRSVM_CONST_MAX_COUNT && vm->constants[slot_num] != NULL;
}

#define CONST_ALLOCATOR(type,name,flag) \
    srsvm_constant_value *srsvm_alloc_const_##name(srsvm_vm *vm, const srsvm_word index, const type value)\
{ \
    srsvm_constant_value *c = NULL; \
    if(index < SRSVM_CONST_MAX_COUNT && !const_slot_in_use(vm, index)){ \
        c = srsvm_const_alloc(flag); \
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
        c = srsvm_const_alloc(STR);
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
