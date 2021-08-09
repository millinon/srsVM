#include "config.h"

#include "srsvm/debug.h"
#include "srsvm/word.h"
#include "srsvm/register.h"
#include "srsvm/value_types.h"
#include "srsvm/opcode-helpers.h"

#include "macro_helpers.h"

#define IMPL_UNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[]) \
    { \
        srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]); \
        srsvm_register *a_reg = register_lookup(vm, thread, &argv[1]); \
        srsvm_word dest_offset = argc < 2 ? 0 : argv[3].value; \
        srsvm_word a_offset = argc < 3 ? 0 : argv[4].value; \
        if(dest_reg != NULL && a_reg != NULL && !fault_on_not_writable(thread, dest_reg)){ \
            ctype a_val, out_val; \
            if(! EVAL2(reg_read,field)(a_reg, &a_val, a_offset)){ \
		thread_set_fault(thread, "Failed to read value from register A"); \
            } else if(fault_cond){ \
		thread_set_fault(thread, fault_message); \
            } else { \
                out_val = (ctype)(expression); \
                if(! EVAL2(load,field)(dest_reg, out_val, dest_offset)){ \
	            thread_set_fault(thread, "Failed to load value into register"); \
                } \
            } \
        } \
    }

#define IMPL_BINARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[]) \
    { \
        srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]); \
        srsvm_register *a_reg = register_lookup(vm, thread, &argv[1]); \
        srsvm_register *b_reg = register_lookup(vm, thread, &argv[2]); \
        srsvm_word dest_offset = argc < 4 ? 0 : argv[3].value; \
        srsvm_word a_offset = argc < 5 ? 0 : argv[4].value; \
        srsvm_word b_offset = argc < 6 ? 0 : argv[5].value; \
        if(dest_reg != NULL && a_reg != NULL && b_reg != NULL && !fault_on_not_writable(thread, dest_reg)){ \
            ctype a_val, b_val, out_val; \
            if(! EVAL2(reg_read,field)(a_reg, &a_val, a_offset)){ \
		thread_set_fault(thread, "Failed to read value from register A"); \
            } else if(! EVAL2(reg_read,field)(b_reg, &b_val, b_offset)){ \
		thread_set_fault(thread, "Failed to read value from register B"); \
            } else if(fault_cond){ \
		thread_set_fault(thread, fault_message); \
            } else { \
                out_val = (ctype)(expression); \
                if(! EVAL2(load,field)(dest_reg, out_val, dest_offset)){ \
	            thread_set_fault(thread, "Failed to load value into register"); \
                } \
            } \
        } \
    }

#define IMPL_TERNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[]) \
    { \
        srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]); \
        srsvm_register *a_reg = register_lookup(vm, thread, &argv[1]); \
        srsvm_register *b_reg = register_lookup(vm, thread, &argv[2]); \
        srsvm_register *c_reg = register_lookup(vm, thread, &argv[3]); \
        srsvm_word dest_offset = argc < 5 ? 0 : argv[4].value; \
        srsvm_word a_offset = argc < 6 ? 0 : argv[5].value; \
        srsvm_word b_offset = argc < 7 ? 0 : argv[6].value; \
        srsvm_word c_offset = argc < 8 ? 0 : argv[7].value; \
        if(dest_reg != NULL && a_reg != NULL && b_reg != NULL && c_reg != NULL && !fault_on_not_writable(thread, dest_reg)){ \
            ctype a_val, b_val, c_val, out_val; \
            if(! EVAL2(reg_read,field)(a_reg, &a_val, a_offset)){ \
		thread_set_fault(thread, "Failed to read value from register A"); \
            } else if(! EVAL2(reg_read,field)(b_reg, &b_val, b_offset)){ \
		thread_set_fault(thread, "Failed to read value from register B"); \
            } else if(! EVAL2(reg_read,field)(c_reg, &c_val, c_offset)){ \
		thread_set_fault(thread, "Failed to read value from register C"); \
            } else if(fault_cond){ \
		    thread_set_fault(thread, fault_message); \
            } else { \
                out_val = (ctype)(expression); \
                if(! EVAL2(load,field)(dest_reg, out_val, dest_offset)){ \
	            thread_set_fault(thread, "Failed to load value into register"); \
                } \
            } \
        } \
    }

#if defined(SRSVM_MOD_MATH_VECTORIZED)

#define IMPL_VECTORIZED_UNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math_VEC,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[]) \
    { \
        srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]); \
        srsvm_register *a_reg = register_lookup(vm, thread, &argv[1]); \
        if(dest_reg != NULL && a_reg != NULL && !fault_on_not_writable(thread, dest_reg)){ \
            ctype a_val; \
            for(size_t i = 0; i < sizeof(a_reg->value.field)/sizeof(ctype); i++){ \
                a_val = a_reg->value.field[i]; \
                if(fault_cond){ \
		    thread_set_fault(thread, fault_message); \
                } else { \
                    dest_reg->value.field[i] = (ctype)(expression); \
                } \
            } \
        } \
    }

#define IMPL_VECTORIZED_BINARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math_VEC,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_arg argv[]) \
    { \
        srsvm_register *dest_reg = register_lookup(vm, thread, &argv[0]); \
        srsvm_register *a_reg = register_lookup(vm, thread, &argv[1]); \
        srsvm_register *b_reg = register_lookup(vm, thread, &argv[2]); \
        if(dest_reg != NULL && a_reg != NULL && b_reg != NULL && !fault_on_not_writable(thread, dest_reg)){ \
            ctype a_val, b_val; \
            for(size_t i = 0; i < sizeof(a_reg->value.field)/sizeof(ctype); i++){ \
                a_val = a_reg->value.field[i]; \
                b_val = b_reg->value.field[i]; \
                if(fault_cond){ \
	            thread_set_fault(thread, fault_message); \
                } else { \
                    dest_reg->value.field[i] = (ctype)(expression); \
                } \
            } \
        } \
    }

#define UNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    IMPL_UNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    IMPL_VECTORIZED_UNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message)

#define BINARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    IMPL_BINARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    IMPL_VECTORIZED_BINARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message)

#endif

#define TERNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    IMPL_TERNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \

#include "math_funcs.h"

#undef UNARY_OPERATOR
#undef BINARY_OPERATOR
#undef TERNARY_OPERATOR
#if defined(SRSVM_MOD_MATH_VECTORIZED)
#undef VECTORIZED_UNARY_OPERATOR
#undef VECTORIZED_BINARY_OPERATOR
#endif
