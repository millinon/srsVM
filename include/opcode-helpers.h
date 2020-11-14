#pragma once

#include <stdlib.h>
#include <string.h>

#include "mmu.h"
#include "opcode.h"
#include "register.h"
#include "vm.h"

static inline bool register_writable(const srsvm_register *reg)
{
    return !(reg->locked || reg->read_only);
}

#define ENSURE_WRITABLE(reg) do { \
    if(! register_writable(reg)){ return false; } \
} while(0)

static inline bool fault_on_not_writable(srsvm_thread *thread, srsvm_register *reg)
{
    if(! register_writable(reg)){
        thread->has_fault = true;
        thread->fault_str = "Attempt to write to a non-writable register";
        return true;
    } else {
        return false;
    }
}

static inline bool set_register_error_bit(srsvm_register *reg, const char* error_str)
{
    ENSURE_WRITABLE(reg);

    reg->error_flag = true;
    reg->error_str = error_str;

    return true;
}

static inline bool clear_reg(srsvm_register *reg)
{
    ENSURE_WRITABLE(reg);

    if(reg->value.str != NULL){
        free(reg->value.str);
    }

    memset(&reg->value, 0, sizeof(reg->value));
    reg->value.str = NULL;
    reg->value.str_len = 0;

    reg->error_flag = false;
    reg->error_str = NULL;

    return true;
}

#define ENSURE_SPACE(type,field) do { \
    if(offset >= (sizeof(reg->value.field) / sizeof(type))){ return false; } \
} while(0)

#define LITERAL_LOAD_HELPER(name,type,field) \
    static inline bool name(srsvm_register *reg, const type value, const srsvm_word offset) \
    { \
        if(! clear_reg(reg)){ return false; } \
        ENSURE_WRITABLE(reg); \
        ENSURE_SPACE(type,field); \
        ((type*)&reg->value.field)[offset] = value; \
        return true; \
    }

#define MEM_LOAD_HELPER(name,type,field) \
    static inline bool name(const srsvm_vm *vm, srsvm_register *reg, const srsvm_ptr ptr, const srsvm_word offset) \
    { \
        if(! clear_reg(reg)){ return false; } \
        ENSURE_WRITABLE(reg); \
        ENSURE_SPACE(type, field); \
        return srsvm_mmu_load(vm->mem_root, ptr, sizeof(type), &reg->value.field + offset); \
    }

#define MEM_STORE_HELPER(name,type,field) \
    static inline bool name(const srsvm_vm *vm, srsvm_register *reg, const srsvm_ptr ptr, const srsvm_word offset) \
    { \
        return srsvm_mmu_store(vm->mem_root, ptr, sizeof(type), &reg->value.field + offset); \
    }

#define MK_HELPERS(type,field) \
    LITERAL_LOAD_HELPER(load_##field, type, field); \
    MEM_LOAD_HELPER(mem_load_##field , type, field); \
    MEM_STORE_HELPER(mem_store_##field , type, field); 

MK_HELPERS(srsvm_word, word);

MK_HELPERS(srsvm_ptr, ptr);
MK_HELPERS(srsvm_ptr_offset, ptr_offset);

MK_HELPERS(bool, bit);

#if WORD_SIZE == 128
MK_HELPERS(unsigned __int128, u128);
MK_HELPERS(__int128, i128);
#endif

#if WORD_SIZE == 128 || WORD_SIZE == 64
MK_HELPERS(uint64_t, u64);
MK_HELPERS(int64_t, i64);

MK_HELPERS(double, f64);
#endif

#if WORD_SIZE == 128 || WORD_SIZE == 64 || WORD_SIZE == 32
MK_HELPERS(uint32_t, u32);
MK_HELPERS(int32_t, i32);

MK_HELPERS(float, f32);
#endif

MK_HELPERS(uint16_t, u16);
MK_HELPERS(int16_t, i16);

MK_HELPERS(uint8_t, u8);
MK_HELPERS(uint8_t, i8);

static inline bool load_str(srsvm_register *reg, const char* value, size_t len)
{
    ENSURE_WRITABLE(reg);

    if(reg->value.str != NULL){
        free(reg->value.str);
    }

    if((reg->value.str = malloc((len + 1) * sizeof(char))) == NULL){
        reg->value.str = NULL;
        reg->value.str_len = 0;
        return false;
    } else {
        memset(reg->value.str, 0, (len + 1) * sizeof(char));
        strncpy(reg->value.str, value, len);
        reg->value.str_len = len;
        return true;
    }
}

static inline bool load_str_len(srsvm_register *reg, const char* value)
{
    return load_str(reg, value, strlen(value));
}

static inline srsvm_register* register_lookup(const srsvm_vm *vm, srsvm_thread *thread, const srsvm_word register_id)
{
    return srsvm_vm_register_lookup(vm, thread, register_id);
}
