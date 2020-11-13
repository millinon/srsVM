#pragma once

#include "forward-decls.h"

#include "impl.h"
#include "register.h"

typedef struct srsvm_spilled_register srsvm_spilled_register;

struct srsvm_spilled_register
{
    srsvm_word index;
    srsvm_register reg;

    srsvm_spilled_register *last;
    srsvm_spilled_register *next;
};

typedef struct srsvm_stack_frame srsvm_stack_frame;

struct srsvm_stack_frame
{
    srsvm_ptr PC;

    srsvm_spilled_register *spilled;
    srsvm_spilled_register *last_spilled;

    srsvm_stack_frame *next;
    srsvm_stack_frame *last;
};

typedef struct
{
   srsvm_stack_frame *frames;
   srsvm_stack_frame *top;
} srsvm_call_stack;

struct srsvm_thread
{
    srsvm_word id;

    srsvm_call_stack call_stack;

    srsvm_ptr PC;

    srsvm_word exit_status;

    bool is_halted;

    bool has_fault;
    const char* fault_str;

    srsvm_thread_native_handle native_handle;
};

srsvm_thread *srsvm_thread_alloc(srsvm_vm *vm, srsvm_word id, srsvm_ptr start_addr);
void srsvm_thread_free(srsvm_vm *vm, srsvm_thread* thread);

bool srsvm_push(srsvm_vm *vm, srsvm_thread *thread, const srsvm_register *reg);
bool srsvm_pop(srsvm_vm *vm, srsvm_thread *thread);

bool srsvm_call(srsvm_vm *vm, srsvm_thread *thread, const srsvm_ptr addr);
bool srsvm_ret(srsvm_vm *vm, srsvm_thread *thread);

#define MAX_THREAD_COUNT (4 * WORD_SIZE)
