#include <math.h>
#include <stdint.h>

#include "srsvm/value_types.h"

#include "macro_helpers.h"

#if !defined(UNARY_OPERATOR)
#error "UNARY_OPERATOR() not defined"
#elif !defined(BINARY_OPERATOR)
#error "BINARY_OPERATOR() not defined"
#elif !defined(TERNARY_OPERATOR)
#error "TERNARY_OPERATOR() not defined"
#endif

#define OP_MIN(type,ctype,field) \
    BINARY_OPERATOR(MIN,type,ctype,field,(a_val < b_val ? a_val : b_val),false,"")
#define OP_MAX(type,ctype,field) \
    BINARY_OPERATOR(MAX,type,ctype,field,(a_val > b_val ? a_val : b_val),false,"")

#define OP_CLAMP(type,ctype,field) \
    TERNARY_OPERATOR(CLAMP,type,ctype,field,(a_val < b_val ? b_val : (a_val > c_val ? c_val : a_val)),false,"")

#define OP_INTEGRAL_ABS(type,ctype,field) \
    UNARY_OPERATOR(ABS,type,ctype,field,(a_val < 0 ? -a_val : a_val),false,"") 
#define OP_ADD(type,ctype,field) \
    BINARY_OPERATOR(ADD,type,ctype,field,(a_val + b_val),false,"")
#define OP_SUB(type,ctype,field) \
    BINARY_OPERATOR(SUB,type,ctype,field,(a_val - b_val), false, "")
#define OP_MUL(type,ctype,field) \
    BINARY_OPERATOR(MUL,type,ctype,field,(a_val * b_val), false, "")
#define OP_DIV(type,ctype,field) \
    BINARY_OPERATOR(DIV,type,ctype,field,(a_val / b_val), b_val == 0, "Division by zero")
#define OP_INTEGRAL_REM(type,ctype,field) \
    BINARY_OPERATOR(REM,type,ctype,field,(a_val % b_val), false, "")

#define OP_FLOATING_ABS(type,ctype,field) \
    UNARY_OPERATOR(ABS,type,ctype,field,(fabs(a_val)), false, "")

#define OP_FLOATING_SQRT(type,ctype,field) \
    UNARY_OPERATOR(SQRT,type,ctype,field,(sqrt(a_val)), false, "")

#define OP_CEIL(type,ctype,field) \
    UNARY_OPERATOR(CEIL,type,ctype,field,(ceil(a_val)), false, "")

#define OP_FLOOR(type,ctype,field) \
    UNARY_OPERATOR(FLOOR,type,ctype,field,(floor(a_val)), false, "")

#define OP_SIN(type,ctype,field) \
    UNARY_OPERATOR(SIN,type,ctype,field,(sin(a_val)), false, "")
#define OP_COS(type,ctype,field) \
    UNARY_OPERATOR(COS,type,ctype,field,(cos(a_val)), false, "")
#define OP_TAN(type,ctype,field) \
    UNARY_OPERATOR(TAN,type,ctype,field,(tan(a_val)), false, "")

#define OP_ASIN(type,ctype,field) \
    UNARY_OPERATOR(ASIN,type,ctype,field,(asin(a_val)), false, "")
#define OP_ACOS(type,ctype,field) \
    UNARY_OPERATOR(ACOS,type,ctype,field,(acos(a_val)), false, "")
#define OP_ATAN(type,ctype,field) \
    UNARY_OPERATOR(ATAN,type,ctype,field,(atan(a_val)), false, "")

#define OP_SINH(type,ctype,field) \
    UNARY_OPERATOR(SINH,type,ctype,field,(sinh(a_val)), false, "")
#define OP_COSH(type,ctype,field) \
    UNARY_OPERATOR(COSH,type,ctype,field,(cosh(a_val)), false, "")
#define OP_TANH(type,ctype,field) \
    UNARY_OPERATOR(TANH,type,ctype,field,(tanh(a_val)), false, "")

#define OP_POW(type,ctype,field) \
    BINARY_OPERATOR(POW,type,ctype,field,(pow(a_val,b_val)), false, "")

#define OP_EXP(type,ctype,field) \
    UNARY_OPERATOR(EXP,type,ctype,field,(exp(a_val)), false, "")

#define OP_LOG(type,ctype,field) \
    UNARY_OPERATOR(LOG,type,ctype,field,(log(a_val)), false, "")
#define OP_LOG10(type,ctype,field) \
    UNARY_OPERATOR(LOG10,type,ctype,field,(log10(a_val)), false, "")


#define UNSIGNED_INTEGRAL_FUNCS(type,ctype,field) \
    OP_ADD(type,ctype,field) \
    OP_SUB(type,ctype,field) \
    OP_MUL(type,ctype,field) \
    OP_DIV(type,ctype,field) \
    OP_INTEGRAL_REM(type,ctype,field) \
    OP_MIN(type,ctype,field) \
    OP_MAX(type,ctype,field) \
    OP_CLAMP(type,ctype,field)

#define SIGNED_INTEGRAL_FUNCS(type,ctype,field) \
    UNSIGNED_INTEGRAL_FUNCS(type,ctype,field) \
    OP_INTEGRAL_ABS(type,ctype,field)

#define FLOAT_FUNCS(type,ctype,field) \
    OP_ADD(type,ctype,field) \
    OP_SUB(type,ctype,field) \
    OP_MUL(type,ctype,field) \
    OP_DIV(type,ctype,field) \
    OP_MIN(type,ctype,field) \
    OP_MAX(type,ctype,field) \
    OP_FLOATING_ABS(type,ctype,field) \
    OP_CLAMP(type,ctype,field) \
    OP_SIN(type,ctype,field) \
    OP_COS(type,ctype,field) \
    OP_TAN(type,ctype,field) \
    OP_ASIN(type,ctype,field) \
    OP_ACOS(type,ctype,field) \
    OP_ATAN(type,ctype,field) \
    OP_SINH(type,ctype,field) \
    OP_COSH(type,ctype,field) \
    OP_TANH(type,ctype,field) \
    OP_FLOATING_SQRT(type,ctype,field) \
    OP_POW(type,ctype,field) \
    OP_LOG(type,ctype,field) \
    OP_LOG10(type,ctype,field) \
    OP_EXP(type,ctype,field) \
    OP_FLOOR(type,ctype,field) \
    OP_CEIL(type,ctype,field)


UNSIGNED_INTEGRAL_FUNCS(U8,uint8_t,u8);
SIGNED_INTEGRAL_FUNCS(I8,int8_t,i8);

UNSIGNED_INTEGRAL_FUNCS(U16,uint16_t,u16);
SIGNED_INTEGRAL_FUNCS(I16,int16_t,i16);

#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
UNSIGNED_INTEGRAL_FUNCS(U32,uint32_t,u32);
SIGNED_INTEGRAL_FUNCS(I32,int32_t,i32);

FLOAT_FUNCS(F32,float,f32);
#endif

#if WORD_SIZE == 64 || WORD_SIZE == 128
UNSIGNED_INTEGRAL_FUNCS(U64,uint64_t,u64);
SIGNED_INTEGRAL_FUNCS(I64,int64_t,i64);

FLOAT_FUNCS(F64,double,f64);
#endif

#if WORD_SIZE == 128
UNSIGNED_INTEGRAL_FUNCS(U128,unsigned __int128,u128);
SIGNED_INTEGRAL_FUNCS(I128,__int128,i128);
#endif
