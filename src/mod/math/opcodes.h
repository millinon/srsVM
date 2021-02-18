#pragma once

#include "config.h"

#include "macro_helpers.h"

#ifndef REGISTER_OPCODE

#error "REGISTER_OPCODE() not defined"

#endif

srsvm_word code = 0;


#if defined(SRSVM_MOD_MATH_VECTORIZED)

#define UNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_message) \
    REGISTER_OPCODE(code++,EVAL2(name,type),2,4); \
    REGISTER_OPCODE(code++,EVAL3(VEC,name,type),2,2);

#define BINARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_essage) \
    REGISTER_OPCODE(code++,EVAL2(name,type),3,6); \
    REGISTER_OPCODE(code++,EVAL3(VEC,name,type),3,3);

#else

#define UNARY_OPERATOR(name,type,ctype8yy,field,expression,fault_cond,fault_message) \
    REGISTER_OPCODE(code++,EVAL2(name,type),2,4);

#define BINARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_essage) \
    REGISTER_OPCODE(code++,EVAL2(name,type),3,6);

#define TERNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_essage) \
    REGISTER_OPCODE(code++,EVAL2(name,type),4,8);

#endif

#define TERNARY_OPERATOR(name,type,ctype,field,expression,fault_cond,fault_essage) \
    REGISTER_OPCODE(code++,EVAL2(name,type),4,8);

#include "math_funcs.h"

#undef UNARY_OPERATOR
#undef BINARY_OPERATOR
#undef TERNARY_OPERATOR
#if defined(SRSVM_MOD_MATH_VECTORIZED)
#undef VECTORIZED_UNARY_OPERATOR
#undef VECTORIZED_BINARY_OPERATOR
#endif
