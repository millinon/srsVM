#pragma once

#include <stdbool.h>

#include "forward-decls.h"

#include "memory.h"
#include "opcode.h"
#include "register.h"
#include "thread.h"

struct srsvm_vm
{
    srsvm_opcode_map *opcode_map;
    srsvm_memory_segment *mem_root;

    srsvm_register *registers[MAX_REGISTER_COUNT];

    srsvm_thread *threads[MAX_THREAD_COUNT];
};

srsvm_vm *srsvm_vm_alloc(srsvm_virtual_memory_desc *memory_layout);
void srsvm_vm_free(srsvm_vm *vm);

bool load_builtin_opcodes(srsvm_vm *vm);

bool srsvm_vm_execute_instruction(srsvm_vm *vm, srsvm_thread *thread, const srsvm_instruction *instruction);

srsvm_thread *srsvm_vm_alloc_thread(srsvm_vm *vm, const srsvm_ptr start_addr);

bool srsvm_vm_start_thread(srsvm_vm *vm, const srsvm_word thread_id);
bool srsvm_vm_join_thread(srsvm_vm *vm, const srsvm_word thread_id);
