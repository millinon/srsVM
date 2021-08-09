#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "srsvm/opcode.h"
#include "srsvm/mmu.h"
#include "srsvm/register.h"
#include "srsvm/vm.h"

static inline void thread_set_fault(srsvm_thread *thread, const char* fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	
	vsnprintf(thread->fault_str, sizeof(thread->fault_str), fmt, args);
		
	va_end(args);
	
	thread->has_fault = true;
}

static inline bool require_arg_type(srsvm_vm *vm, srsvm_thread *thread, const srsvm_arg *arg, const srsvm_arg_type type_mask)
{
	bool is_valid = false;

	if(arg != NULL){
		if((arg->type & type_mask) != 0){
			is_valid = true;
		} else {
			thread_set_fault(thread, "Invalid argument type for opcode: expected %u, found %u", type_mask, arg->type);
		}
	}

	return is_valid;
}

static inline bool resolve_arg_word(srsvm_vm *vm, srsvm_thread *thread, const srsvm_arg *arg, srsvm_word *dest, bool fault_on_failure)
{
	bool success = false;

	if(dest != NULL){
		*dest = 0;
	}

	if(arg != NULL){
		switch(arg->type){
			case SRSVM_ARG_TYPE_WORD:
				if(dest != NULL){
					*dest = arg->value;
				}

				success = true;
				break;

			case SRSVM_ARG_TYPE_REGISTER:
				if(arg->value < SRSVM_REGISTER_MAX_COUNT && vm->registers[arg->value] != NULL){
					if(dest != NULL){
						*dest = vm->registers[arg->value]->value.word;
					}

					success = true;
				}
				break;

			case SRSVM_ARG_TYPE_CONSTANT:
				if(arg->value < SRSVM_CONST_MAX_COUNT && vm->constants[arg->value] != NULL &&
						vm->constants[arg->value]->type == SRSVM_TYPE_WORD){
					if(dest != NULL){
						*dest = vm->constants[arg->value]->word;
					}
					success = true;
				}
				break;

		}
	}

	if(! success && fault_on_failure){
		thread_set_fault(thread, "Failed to resolve argument to word value");
	}

	return success;
}

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
		thread_set_fault(thread, "Attempted to write to non-writable register %s", reg->name);
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

#define REG_READ_HELPER(name,type,field) \
	static inline bool name(srsvm_register *reg, type* value, const srsvm_word offset) \
{ \
	ENSURE_SPACE(type,field); \
	*value = *(type*)(&reg->value.field + offset); \
	return true; \
}

#define LITERAL_LOAD_HELPER(name,type,field) \
	static inline bool name(srsvm_register *reg, const type value, const srsvm_word offset) \
{ \
	ENSURE_WRITABLE(reg); \
	ENSURE_SPACE(type,field); \
	((type*)&reg->value.field)[offset] = value; \
	return true; \
}

#define MEM_LOAD_HELPER(name,type,field) \
	static inline bool name(const srsvm_vm *vm, srsvm_register *reg, const srsvm_ptr ptr, const srsvm_word offset) \
{ \
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
	REG_READ_HELPER(reg_read_##field, type, field); \
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
MK_HELPERS(int8_t, i8);

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
		//srsvm_strncpy(reg->value.str, value, len+1);
		strcpy(reg->value.str, value);
		reg->value.str_len = len;
		return true;
	}
}

static inline bool mem_store_str(const srsvm_vm *vm, srsvm_register *reg, const srsvm_ptr ptr, const srsvm_word offset)
{
	return srsvm_mmu_store(vm->mem_root, ptr, (reg->value.str_len + 1) * sizeof(char), &reg->value.str);
}

static inline bool mem_load_str(const srsvm_vm *vm, srsvm_register *reg, const srsvm_ptr ptr, const srsvm_word offset)
{
	srsvm_memory_segment *seg = srsvm_mmu_locate(vm->mem_root, ptr);

	if(seg != NULL){
		for(size_t str_len = 0; str_len < seg->literal_sz - (ptr - seg->literal_start); str_len++){
			char *lit_ptr = ((char*) seg->literal_memory) + str_len;

			if(*lit_ptr == 0){
				reg->value.str = srsvm_strdup(lit_ptr - str_len);
				reg->value.str_len = (srsvm_word) str_len;
				return true;
			}
		}
	}

	return false;
}

static inline bool load_str_len(srsvm_register *reg, const char* value)
{
	return load_str(reg, value, strlen(value));
}

static inline srsvm_register* register_lookup(const srsvm_vm *vm, srsvm_thread *thread, const srsvm_arg* arg)
{
	srsvm_register *reg = NULL;

	if(arg->type != SRSVM_ARG_TYPE_REGISTER){
		thread_set_fault(thread, "Attempt to use a non-register value " PRINT_WORD_HEX " as a register", arg->value);
	} else {
		srsvm_word register_id = arg->value;

		if(register_id< SRSVM_REGISTER_MAX_COUNT){
			reg = vm->registers[register_id];

			if(reg == NULL){
				thread_set_fault(thread, "Attempt to access an unallocated register with index " PRINT_WORD_HEX, register_id);
			}
		} else {
			thread_set_fault(thread, "Attempt to access register at invalid index " PRINT_WORD_HEX, register_id);
		}
	}

	return reg;
}
