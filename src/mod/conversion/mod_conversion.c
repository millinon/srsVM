#include <stdio.h>

#include "config.h"

#include "srsvm/debug.h"
#include "srsvm/word.h"
#include "srsvm/register.h"
#include "srsvm/value_types.h"
#include "srsvm/opcode-helpers.h"

#include "macro_helpers.h"

#define CAST_CONVERTER(name,type_from,ctype_from,field_from,type_to,ctype_to,field_to) \
    void EVAL3(conversion,name,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[]) \
    { \
        srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]); \
        srsvm_register *src_reg = register_lookup(vm, thread, &argv[1]); \
        srsvm_word dest_offset = argc < 3 ? 0 : argv[2].value; \
        srsvm_word src_offset = argc < 4 ? 0 : argv[3].value; \
        if(dest_reg != NULL && src_reg != NULL && !fault_on_not_writable(thread, dest_reg)){ \
            ctype_from val_in; \
            ctype_to val_out; \
            if(! EVAL2(reg_read,field_from)(src_reg, &val_in, src_offset)){ \
		thread_set_fault(thread, "Failed to read value from register"); \
            } else { \
                val_out = (ctype_to)(val_in); \
                if(! EVAL2(load,field_to)(dest_reg, val_out, dest_offset)){ \
		    thread_set_fault(thread, "Failed to load value into register"); \
                } \
            } \
        } \
    }

#define PARSE_CONVERTER(name,type_to,ctype_to,field_to,fmt) \
    void EVAL3(conversion,name,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[]) \
    { \
        srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]); \
        srsvm_register *src_reg = register_lookup(vm, thread, &argv[1]); \
        srsvm_word dest_offset = argc < 3 ? 0 : argv[2].value; \
        if(dest_reg != NULL && src_reg != NULL && !fault_on_not_writable(thread, dest_reg)){ \
            ctype_to val_out; \
            size_t parsed_chars = 0; \
            if(src_reg->value.str == NULL){ \
                set_register_error_bit(dest_reg, "Attempted to parse a null string"); \
            } else { \
                if(sscanf(fmt "%n", src_reg->value.str, &val_out, &parsed_chars) < 1 || parsed_chars < strlen(src_reg->value.str)){ \
                    set_register_error_bit(dest_reg, #type_to " parse failed"); \
                } else if(! EVAL2(load,field_to)(dest_reg, val_out, dest_offset)){ \
		    thread_set_fault(thread, "Failed to load value into register"); \
                } \
            } \
        } \
    }




#define TOSTR_CONVERTER(name,type_from,ctype_from,field_from,fmt) \
    void EVAL3(conversion,name,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[]) \
    { \
        static char buf[1024]; \
        srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]); \
        srsvm_register *src_reg = register_lookup(vm, thread, &argv[1]); \
        srsvm_word src_offset = argc < 3 ? 0 : argv[2].value; \
        if(dest_reg != NULL && src_reg != NULL && !fault_on_not_writable(thread, dest_reg)){ \
            memset(buf, 0, sizeof(buf)); \
            ctype_from val_in; \
            if(! EVAL2(reg_read,field_from)(src_reg, &val_in, src_offset)){ \
		thread_set_fault(thread, "Failed to read value from register"); \
            } else { \
                int out_len = snprintf(buf, sizeof(buf), fmt, val_in); \
                if(out_len <= 0){ \
                    set_register_error_bit(dest_reg, "Failed to serialize " #type_from); \
                } else if(! load_str(dest_reg, buf, (size_t) out_len)){ \
		    thread_set_fault(thread, "Failed to load value into register"); \
                } \
            } \
        } \
    }


#include "conversion_funcs.h"

#undef IMPL_UINT128_PARSERS
#undef CAST_CONVERTER
#undef PARSE_CONVERTER
#undef TOSTR_CONVERTER

#if WORD_SIZE == 128
void conversion_PARSE_U128_128(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[])
{
    srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]);
    srsvm_register *src_reg = register_lookup(vm, thread, &argv[1]);
    srsvm_word dest_offset = argc < 3 ? 0 : argv[2].value;
    if(dest_reg != NULL && src_reg != NULL && !fault_on_not_writable(thread, dest_reg)){
        if(src_reg->value.str == NULL){ \
            set_register_error_bit(dest_reg, "Attempted to parse a null string");
        } else {
            char *src = src_reg->value.str;
            size_t src_len = strlen(src);

            unsigned __int128 val_out;

            uint64_t upper = 0, lower = 0;

            bool parse_success = false;

            if(src_len <= 32){
                int parsed_chars = 0;
                if(sscanf(src, "0x%" SCNx64 "%n", &lower, &parsed_chars) < 1 || parsed_chars < src_len){
                    set_register_error_bit(dest_reg, "U128 parse failed");
                } else {
                    val_out = lower;
                    parse_success = true;
                }
            } else {
                char lower_buf[33] = { 0 };
                char upper_buf[33] = { 0 };
                size_t upper_len = src_len - 32;
                size_t lower_len = src_len - upper_len;
                int lower_parsed_chars = 0;
                int upper_parsed_chars = 0;

                strncpy(lower_buf, src + upper_len, 32);
                strncpy(upper_buf, src, upper_len);

                if(sscanf(lower_buf, "%" SCNx64 "%n", &lower, &lower_parsed_chars) < 1 || lower_parsed_chars < lower_len || 
                        sscanf(upper_buf, "0x%" SCNx64 "%n", &upper, &upper_parsed_chars) < 1 || upper_parsed_chars < upper_len){
                    set_register_error_bit(dest_reg, "U128 parse failed");
                } else parse_success = true;
            }

            if(parse_success){
                if(! load_u128(dest_reg, val_out, dest_offset)){
                    thread_set_fault(thread, "Failed to load value into register");
                }
            }
        }
    }
}

void conversion_U128_TO_STR_128(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[])
{
    static char buf[1024];
    srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]);
    srsvm_register *src_reg = register_lookup(vm, thread, &argv[1]);
    srsvm_word src_offset = argc < 3 ? 0 : argv[2].value;
    if(dest_reg != NULL && src_reg != NULL && !fault_on_not_writable(thread, dest_reg)){
        memset(buf, 0, sizeof(buf));
        unsigned __int128 val_in;
        if(! reg_read_u128(src_reg, &val_in, src_offset)){
		thread_set_fault(thread, "Failed to read value from register");
        } else {
            int out_len = 0;

            uint64_t lower = (uint64_t) (val_in & UINT64_MAX);
            uint64_t upper = (uint64_t) ((val_in >> 64) & UINT64_MAX);

            if(upper == 0){
                out_len = snprintf(buf, sizeof(buf), "0x%" PRIx64, lower);
            } else {
                out_len = snprintf(buf, sizeof(buf), "0x%" PRIx64 "%" PRIx64, upper, lower);
            }

            if(out_len <= 0){
                set_register_error_bit(dest_reg, "Failed to serialize U128");
            } else if(! load_str(dest_reg, buf, (size_t) out_len)){
                thread_set_fault(thread, "Failed to load value into register");
            }
        }
    }
}
#endif
