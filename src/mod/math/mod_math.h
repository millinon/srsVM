#pragma once

#include "config.h"

#if !defined(WORD_SIZE)

#error "WORD_SIZE not defined"

#endif

#if defined(SRSVM_MOD_MATH_VECTORIZED)
#define UNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]); \
    void EVAL4(math_VEC,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]);

#define BINARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]); \
    void EVAL4(math_VEC,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]);

#else
#define UNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]);

#define BINARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]);

#define TERNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]);

#endif

#define TERNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    void EVAL4(math,name,type,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]);

#include "math_funcs.h"

#undef UNARY_OPERATOR
#undef BINARY_OPERATOR
#undef TERNARY_OPERATOR

#if defined(SRSVM_MOD_MATH_VECTORIZED)
#undef VECTORIZED_UNARY_OPERATOR
#undef VECTORIZED_BINARY_OPERATOR
#endif
