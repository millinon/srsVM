#pragma once

#include <stdbool.h>

#include "forward-decls.h"

#include "memory.h"
#include "module.h"
#include "opcode.h"
#include "register.h"
#include "thread.h"

struct srsvm_vm
{
    srsvm_opcode_map *opcode_map;
    srsvm_module_map *module_map;

    srsvm_memory_segment *mem_root;

    srsvm_register *registers[SRSVM_REGISTER_MAX_COUNT];

    srsvm_thread *threads[SRSVM_THREAD_MAX_COUNT];

    srsvm_module *modules[SRSVM_MODULE_MAX_COUNT];
    char** module_search_path;

};

srsvm_vm *srsvm_vm_alloc(srsvm_virtual_memory_desc *memory_layout);
void srsvm_vm_free(srsvm_vm *vm);

bool load_builtin_opcodes(srsvm_vm *vm);

bool srsvm_vm_execute_instruction(srsvm_vm *vm, srsvm_thread *thread, const srsvm_instruction *instruction);

srsvm_thread *srsvm_vm_alloc_thread(srsvm_vm *vm, const srsvm_ptr start_addr);

bool srsvm_vm_start_thread(srsvm_vm *vm, const srsvm_word thread_id);
bool srsvm_vm_join_thread(srsvm_vm *vm, const srsvm_word thread_id);

void srsvm_vm_set_module_search_path(srsvm_vm *vm, const char* search_path);
srsvm_module *srsvm_vm_load_module(srsvm_vm *vm, const char* module_name);
void srsvm_vm_unload_module(srsvm_vm *vm, srsvm_module *mod);
