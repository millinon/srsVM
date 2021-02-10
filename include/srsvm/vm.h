#pragma once

#include <stdbool.h>

#include "srsvm/constant.h"
#include "srsvm/forward-decls.h"
#include "srsvm/map.h"
#include "srsvm/memory.h"
#include "srsvm/module.h"
#include "srsvm/opcode.h"
#include "srsvm/program.h" 
#include "srsvm/register.h"
#include "srsvm/thread.h"

struct srsvm_vm
{
    srsvm_opcode_map *opcode_map;
    srsvm_string_map *module_map;

    srsvm_memory_segment *mem_root;

    srsvm_register *registers[SRSVM_REGISTER_MAX_COUNT];

    srsvm_thread *threads[SRSVM_THREAD_MAX_COUNT];

    srsvm_module *modules[SRSVM_MODULE_MAX_COUNT];
    char** module_search_path;

    srsvm_constant_value *constants[SRSVM_CONST_MAX_COUNT];

    bool has_program_loaded;

    srsvm_thread *main_thread;
};

srsvm_vm *srsvm_vm_alloc(void);
void srsvm_vm_free(srsvm_vm *vm);

bool srsvm_vm_load_program(srsvm_vm *vm, const srsvm_program *program);

srsvm_register *srsvm_vm_register_alloc(srsvm_vm *vm, const char* name, const srsvm_word index);
srsvm_register *srsvm_vm_register_lookup(const srsvm_vm *vm, srsvm_thread* thread, const srsvm_word index);

//bool load_builtin_opcodes(srsvm_vm *vm);

bool srsvm_vm_execute_instruction(srsvm_vm *vm, srsvm_thread *thread, const srsvm_instruction *instruction);

srsvm_thread *srsvm_vm_alloc_thread(srsvm_vm *vm, const srsvm_ptr start_addr);

bool srsvm_vm_start_thread(srsvm_vm *vm, const srsvm_word thread_id);
bool srsvm_vm_join_thread(srsvm_vm *vm, const srsvm_word thread_id);

void srsvm_vm_set_module_search_path(srsvm_vm *vm, const char* search_path);
srsvm_module *srsvm_vm_load_module(srsvm_vm *vm, const char* module_name);
srsvm_module *srsvm_vm_load_module_slot(srsvm_vm *vm, const char* module_name, const srsvm_word slot_num);
void srsvm_vm_unload_module(srsvm_vm *vm, srsvm_module *mod);
srsvm_opcode *srsvm_vm_load_module_opcode(srsvm_vm *vm, srsvm_module *mod, const srsvm_word opcode);

#define CONST_ALLOCATOR(type,name) \
    srsvm_constant_value* srsvm_vm_alloc_const_##name(srsvm_vm *vm, const srsvm_word index, const type value)
CONST_ALLOCATOR(srsvm_word, word);
CONST_ALLOCATOR(srsvm_ptr, ptr);
CONST_ALLOCATOR(srsvm_ptr_offset, ptr_offset);
CONST_ALLOCATOR(bool, bit);
CONST_ALLOCATOR(uint8_t, u8);
CONST_ALLOCATOR(int8_t, i8);
CONST_ALLOCATOR(uint16_t, u16);
CONST_ALLOCATOR(int16_t, i16);
#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
CONST_ALLOCATOR(uint32_t, u32);
CONST_ALLOCATOR(int32_t, i32);
CONST_ALLOCATOR(float, f32);
#endif
#if WORD_SIZE == 64 || WORD_SIZE == 128
CONST_ALLOCATOR(uint64_t, u64);
CONST_ALLOCATOR(int64_t, i64);
CONST_ALLOCATOR(double, f64);
#endif
#if WORD_SIZE == 128
CONST_ALLOCATOR(unsigned __int128, u128);
CONST_ALLOCATOR(__int128, i128);
#endif
#undef CONST_ALLOCATOR
srsvm_constant_value *srsvm_vm_alloc_const_str(srsvm_vm *vm, const srsvm_word index, const char* value);

bool srsvm_vm_load_const(srsvm_vm *vm, srsvm_register *dest_reg, const srsvm_word index, const srsvm_word offset);
