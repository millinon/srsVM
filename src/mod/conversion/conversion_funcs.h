#include <stdint.h>
#include <inttypes.h>

#include "srsvm/value_types.h"

#include "macro_helpers.h"

#if !defined(CAST_CONVERTER)
#error "CAST_CONVERTER() not defined"
#elif !defined(PARSE_CONVERTER)
#error "PARSE_CONVERTER() not defined"
#elif !defined(TOSTR_CONVERTER)
#error "TOSTR_CONVERTER() not defined"
#endif


#define U8_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(U8,TO,type),U8,uint8_t,u8,type,ctype,field)

#define I8_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(I8,TO,type),I8,int8_t,i8,type,ctype,field)

#define U16_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(U16,TO,type),U16,uint16_t,u16,type,ctype,field)

#define I16_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(I16,TO,type),I16,int16_t,i16,type,ctype,field)

#if WORD_SIZE == 16
#define U32_TO(type,field,ctype)    
#define I32_TO(type,field,ctype)
#define F32_TO(type,field,ctype)
#else
#define U32_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(U32,TO,type),U32,uint32_t,u32,type,ctype,field)

#define I32_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(I32,TO,type),I32,int32_t,i32,type,ctype,field)

#define F32_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(F32,TO,type),F32,float,f32,type,ctype,field)
#endif

#if WORD_SIZE == 16 || WORD_SIZE == 32
#define U64_TO(type,field,ctype)
#define I64_TO(type,field,ctype)
#define F64_TO(type,field,ctype)
#else
#define U64_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(U64,TO,type),U64,uint64_t,u64,type,ctype,field)

#define I64_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(I64,TO,type),I64,int64_t,i64,type,ctype,field)

#define F64_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(F64,TO,type),F64,double,f64,type,ctype,field)
#endif

#if WORD_SIZE == 16 || WORD_SIZE == 32 || WORD_SIZE == 64
#define U128_TO(type,field,ctype)
#define I128_TO(type,field,ctype)
#else
#define U128_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(U128,TO,type),U128,unsigned __int128,u128,type,ctype,field)

#define I128_TO(type,field,ctype) \
    CAST_CONVERTER(EVAL3(I128,TO,type),I128,__int128,i128,type,ctype,field)
#endif

#define U8_CONVERTERS_BASE \
    U8_TO(I8,i8,int8_t); \
    U8_TO(U16,u16,uint16_t); \
    U8_TO(I16,i16,int16_t); \
    PARSE_CONVERTER(PARSE_U8,U8,uint8_t,u8,"%" SCNu8); \
    PARSE_CONVERTER(PARSE_U8_HEX,U8,uint8_t,u8,"0x%" SCNx8); \
    TOSTR_CONVERTER(U8_TO_STR,U8,uint8_t,u8,"%" PRIu8); \
    TOSTR_CONVERTER(U8_TO_STR_HEX,U8,uint8_t,u8,"0x%" PRIx8);

#define I8_CONVERTERS_BASE \
    I8_TO(U8,u8,uint8_t); \
    I8_TO(U16,u16,uint16_t); \
    I8_TO(I16,i16,int16_t); \
    PARSE_CONVERTER(PARSE_I8,I8,int8_t,i8,"%" SCNi8); \
    TOSTR_CONVERTER(I8_TO_STR,I8,int8_t,i8,"%" PRIi8);

#define U16_CONVERTERS_BASE \
    U16_TO(I8,i8,int8_t); \
    U16_TO(U8,u8,uint8_t); \
    U16_TO(I16,i16,int16_t); \
    PARSE_CONVERTER(PARSE_U16,U16,uint16_t,u16,"%" SCNu16); \
    PARSE_CONVERTER(PARSE_U16_HEX,U16,uint16_t,u16,"0x%" SCNx16); \
    TOSTR_CONVERTER(U16_TO_STR,U16,uint16_t,u16,"%" PRIu16); \
    TOSTR_CONVERTER(U16_TO_STR_HEX,U16,uint16_t,u16,"0x%" PRIx16);

#define I16_CONVERTERS_BASE \
    I16_TO(I8,i8,int8_t); \
    I16_TO(U8,u8,uint8_t); \
    I16_TO(U16,u16,uint16_t); \
    PARSE_CONVERTER(PARSE_I16,I16,int16_t,i16,"%" SCNi16); \
    TOSTR_CONVERTER(I16_TO_STR,I16,int16_t,i16,"%" PRIi16);

#if WORD_SIZE == 128
#define U8_CONVERTERS U8_CONVERTERS_BASE \
    U8_TO(U32,u32,uint32_t); \
    U8_TO(I32,i32,int32_t); \
    U8_TO(F32,f32,float); \
    U8_TO(U64,u64,uint64_t); \
    U8_TO(I64,i64,int64_t); \
    U8_TO(F64,f64,double); \
    U8_TO(U128,u128,unsigned __int128); \
    U8_TO(I128,i128,__int128);
#define I8_CONVERTERS I8_CONVERTERS_BASE \
    I8_TO(U32,u32,uint32_t); \
    I8_TO(I32,i32,int32_t); \
    I8_TO(F32,f32,float); \
    I8_TO(U64,u64,uint64_t); \
    I8_TO(I64,i64,int64_t); \
    I8_TO(F64,f64,double); \
    I8_TO(U128,u128,unsigned __int128); \
    I8_TO(I128,i128,__int128);
#define U16_CONVERTERS U16_CONVERTERS_BASE \
    U16_TO(U32,u32,uint32_t); \
    U16_TO(I32,i32,int32_t); \
    U16_TO(F32,f32,float); \
    U16_TO(U64,u64,uint64_t); \
    U16_TO(I64,i64,int64_t); \
    U16_TO(F64,f64,double); \
    U16_TO(U128,u128,unsigned __int128); \
    U16_TO(I128,i128,__int128);
#define I16_CONVERTERS I16_CONVERTERS_BASE \
    I16_TO(U32,u32,uint32_t); \
    I16_TO(I32,i32,int32_t); \
    I16_TO(F32,f32,float); \
    I16_TO(U64,u64,uint64_t); \
    I16_TO(I64,i64,int64_t); \
    I16_TO(F64,f64,double); \
    I16_TO(U128,u128,unsigned __int128); \
    I16_TO(I128,i128,__int128);
#elif WORD_SIZE == 64
#define U8_CONVERTERS U8_CONVERTERS_BASE \
    U8_TO(U32,u32,uint32_t); \
    U8_TO(I32,i32,int32_t); \
    U8_TO(F32,f32,float); \
    U8_TO(U64,u64,uint64_t); \
    U8_TO(I64,i64,int64_t); \
    U8_TO(F64,f64,double);
#define I8_CONVERTERS I8_CONVERTERS_BASE \
    I8_TO(U32,u32,uint32_t); \
    I8_TO(I32,i32,int32_t); \
    I8_TO(F32,f32,float); \
    I8_TO(U64,u64,uint64_t); \
    I8_TO(I64,i64,int64_t); \
    I8_TO(F64,f64,double);
#define U16_CONVERTERS U16_CONVERTERS_BASE \
    U16_TO(U32,u32,uint32_t); \
    U16_TO(I32,i32,int32_t); \
    U16_TO(F32,f32,float); \
    U16_TO(U64,u64,uint64_t); \
    U16_TO(I64,i64,int64_t); \
    U16_TO(F64,f64,double);
#define I16_CONVERTERS I16_CONVERTERS_BASE \
    I16_TO(U32,u32,uint32_t); \
    I16_TO(I32,i32,int32_t); \
    I16_TO(F32,f32,float); \
    I16_TO(U64,u64,uint64_t); \
    I16_TO(I64,i64,int64_t); \
    I16_TO(F64,f64,double);
#elif WORD_SIZE == 32
#define U8_CONVERTERS U8_CONVERTERS_BASE \
    U8_TO(U32,u32,uint32_t); \
    U8_TO(I32,i32,int32_t); \
    U8_TO(F32,f32,float);
#define I8_CONVERTERS I8_CONVERTERS_BASE \
    I8_TO(U32,u32,uint32_t); \
    I8_TO(I32,i32,int32_t); \
    I8_TO(F32,f32,float);
#define U16_CONVERTERS U16_CONVERTERS_BASE \
    U16_TO(U32,u32,uint32_t); \
    U16_TO(I32,i32,int32_t); \
    U16_TO(F32,f32,float);
#define I16_CONVERTERS I16_CONVERTERS_BASE \
    I16_TO(U32,u32,uint32_t); \
    I16_TO(I32,i32,int32_t); \
    I16_TO(F32,f32,float);
#else
#define U8_CONVERTERS U8_CONVERTERS_BASE
#define I8_CONVERTERS I8_CONVERTERS_BASE
#define U16_CONVERTERS U16_CONVERTERS_BASE
#define I16_CONVERTERS I16_CONVERTERS_BASE
#endif

#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128

#define U32_CONVERTERS_BASE \
    U32_TO(I8,i8,int8_t); \
    U32_TO(U8,u8,uint8_t); \
    U32_TO(U16,u16,uint16_t); \
    U32_TO(I16,i16,int16_t); \
    U32_TO(I32,i32,int32_t); \
    U32_TO(F32,f32,float); \
    PARSE_CONVERTER(PARSE_U32,U32,uint32_t,u32,"%" SCNu32); \
    PARSE_CONVERTER(PARSE_U32_HEX,U32,uint32_t,u32,"0x%" SCNx32); \
    TOSTR_CONVERTER(U32_TO_STR,U32,uint32_t,u32,"%" PRIu32); \
    TOSTR_CONVERTER(U32_TO_STR_HEX,U32,uint32_t,u32,"0x%" PRIx32);

#define I32_CONVERTERS_BASE \
    I32_TO(I8,i8,int8_t); \
    I32_TO(U8,u8,uint8_t); \
    I32_TO(U16,u16,uint16_t); \
    I32_TO(I16,i16,int16_t); \
    I32_TO(U32,u32,uint32_t); \
    I32_TO(F32,f32,float); \
    PARSE_CONVERTER(PARSE_I32,I32,int32_t,i32,"%" SCNi32); \
    TOSTR_CONVERTER(I32_TO_STR,I32,int32_t,i32,"%" PRIi32); \


#define F32_CONVERTERS_BASE \
    F32_TO(I8,i8,int8_t); \
    F32_TO(U8,u8,uint8_t); \
    F32_TO(U16,u16,uint16_t); \
    F32_TO(I16,i16,int16_t); \
    F32_TO(U32,u32,uint32_t); \
    F32_TO(I32,i32,int32_t); \
    PARSE_CONVERTER(PARSE_F32,F32,float,f32,"%f"); \
    TOSTR_CONVERTER(F32_TO_STR,F32,float,f32,"%f"); 

#if WORD_SIZE == 128
#define U32_CONVERTERS U32_CONVERTERS_BASE \
    U32_TO(U64,u64,uint64_t); \
    U32_TO(I64,i64,int64_t); \
    U32_TO(F64,f64,double); \
    U32_TO(U128,u128,unsigned __int128); \
    U32_TO(I128,i128,__int128);
#define I32_CONVERTERS I32_CONVERTERS_BASE \
    I32_TO(U64,u64,uint64_t); \
    I32_TO(I64,i64,int64_t); \
    I32_TO(F64,f64,double); \
    I32_TO(U128,u128,unsigned __int128); \
    I32_TO(I128,i128,__int128);
#define F32_CONVERTERS F32_CONVERTERS_BASE \
    F32_TO(U64,u64,uint64_t); \
    F32_TO(I64,i64,int64_t); \
    F32_TO(F64,f64,double); \
    F32_TO(U128,u128,unsigned __int128); \
    F32_TO(I128,i128,__int128);
#elif WORD_SIZE == 64
#define U32_CONVERTERS U32_CONVERTERS_BASE \
    U32_TO(U64,u64,uint64_t); \
    U32_TO(I64,i64,int64_t); \
    U32_TO(F64,f64,double);
#define I32_CONVERTERS I32_CONVERTERS_BASE \
    I32_TO(U64,u64,uint64_t); \
    I32_TO(I64,i64,int64_t); \
    I32_TO(F64,f64,double);
#define F32_CONVERTERS F32_CONVERTERS_BASE \
    F32_TO(U64,u64,uint64_t); \
    F32_TO(I64,i64,int64_t); \
    F32_TO(F64,f64,double);
#else
#define U32_CONVERTERS U32_CONVERTERS_BASE
#define I32_CONVERTERS I32_CONVERTERS_BASE
#define F32_CONVERTERS F32_CONVERTERS_BASE
#endif

U32_CONVERTERS
I32_CONVERTERS
F32_CONVERTERS

#if WORD_SIZE == 64 || WORD_SIZE == 128

#define U64_CONVERTERS_BASE \
    U64_TO(I8,i8,int8_t); \
    U64_TO(U8,u8,uint8_t); \
    U64_TO(U16,u16,uint16_t); \
    U64_TO(I16,i16,int16_t); \
    U64_TO(U32,u32,uint32_t); \
    U64_TO(I32,i32,int32_t); \
    U64_TO(F32,f32,float); \
    U64_TO(I64,i64,int64_t); \
    U64_TO(F64,f64,double); \
    PARSE_CONVERTER(PARSE_U64,U64,uint64_t,u64,"%" SCNu64); \
    PARSE_CONVERTER(PARSE_U64_HEX,U64,uint64_t,u64,"0x%" SCNx64); \
    TOSTR_CONVERTER(U64_TO_STR,U64,uint64_t,u64,"%" PRIu64); \
    TOSTR_CONVERTER(U64_TO_STR_HEX,U64,uint64_t,u64,"0x%" PRIx64);

#define I64_CONVERTERS_BASE \
    I64_TO(I8,i8,int8_t); \
    I64_TO(U8,u8,uint8_t); \
    I64_TO(U16,u16,uint16_t); \
    I64_TO(I16,i16,int16_t); \
    I64_TO(U32,u32,uint32_t); \
    I64_TO(I32,i32,int32_t); \
    I64_TO(F32,f32,float); \
    I64_TO(U64,u64,uint64_t); \
    I64_TO(F64,f64,double); \
    PARSE_CONVERTER(PARSE_I64,I64,int64_t,i64,"%" SCNi64); \
    TOSTR_CONVERTER(I64_TO_STR,I64,int64_t,i64,"%" PRIi64); \


#define F64_CONVERTERS_BASE \
    F64_TO(I8,i8,int8_t); \
    F64_TO(U8,u8,uint8_t); \
    F64_TO(U16,u16,uint16_t); \
    F64_TO(I16,i16,int16_t); \
    F64_TO(U32,u32,uint32_t); \
    F64_TO(I32,i32,int32_t); \
    F64_TO(F32,f32,float); \
    F64_TO(U64,u64,uint64_t); \
    F64_TO(I64,i64,int64_t); \
    PARSE_CONVERTER(PARSE_F64,F64,double,f64,"%f"); \
    TOSTR_CONVERTER(F64_TO_STR,F64,double,f64,"%f"); 
    
#if WORD_SIZE == 128
#define U64_CONVERTERS U64_CONVERTERS_BASE \
    U64_TO(U128,u128,unsigned __int128); \
    U64_TO(I128,i128,__int128);
#define I64_CONVERTERS I64_CONVERTERS_BASE \
    I64_TO(U128,u128,unsigned __int128); \
    I64_TO(I128,i128,__int128);
#define F64_CONVERTERS F64_CONVERTERS_BASE \
    F64_TO(U128,u128,unsigned __int128); \
    F64_TO(I128,i128,__int128);
#else
#define U64_CONVERTERS U64_CONVERTERS_BASE
#define I64_CONVERTERS I64_CONVERTERS_BASE
#define F64_CONVERTERS F64_CONVERTERS_BASE
#endif

U64_CONVERTERS
I64_CONVERTERS
F64_CONVERTERS

#if WORD_SIZE == 128

#if defined(IMPL_U128_PARSERS)
#define U128_CONVERTERS \
    U128_TO(I8,i8,int8_t); \
    U128_TO(U8,u8,uint8_t); \
    U128_TO(U16,u16,uint16_t); \
    U128_TO(I16,i16,int16_t); \
    U128_TO(U32,u32,uint32_t); \
    U128_TO(I32,i32,int32_t); \
    U128_TO(F32,f32,float); \
    U128_TO(U64,u64,uint64_t); \
    U128_TO(I64,i64,int64_t); \
    U128_TO(F64,f64,double); \
    U128_TO(I128,i128,__int128); \
    PARSE_CONVERTER(PARSE_U128,U128,,,); \
    TOSTR_CONVERTER(U128_TO_STR,U128,,,); 
#else
#define U128_CONVERTERS \
    U128_TO(I8,i8,int8_t); \
    U128_TO(U8,u8,uint8_t); \
    U128_TO(U16,u16,uint16_t); \
    U128_TO(I16,i16,int16_t); \
    U128_TO(U32,u32,uint32_t); \
    U128_TO(I32,i32,int32_t); \
    U128_TO(F32,f32,float); \
    U128_TO(U64,u64,uint64_t); \
    U128_TO(I64,i64,int64_t); \
    U128_TO(F64,f64,double); \
    U128_TO(I128,i128,__int128);
#endif

#define I128_CONVERTERS \
    I128_TO(I8,i8,int8_t); \
    I128_TO(U8,u8,uint8_t); \
    I128_TO(U16,u16,uint16_t); \
    I128_TO(I16,i16,int16_t); \
    I128_TO(U32,u32,uint32_t); \
    I128_TO(I32,i32,int32_t); \
    I128_TO(F32,f32,float); \
    I128_TO(U64,u64,uint64_t); \
    I128_TO(I64,i64,int64_t); \
    I128_TO(F64,f64,double); \
    I128_TO(U128,u128,unsigned __int128);

U128_CONVERTERS
I128_CONVERTERS

#endif

#endif

#endif

U8_CONVERTERS
I8_CONVERTERS
U16_CONVERTERS
I16_CONVERTERS

#undef U8_TO
#undef I8_TO
#undef U16_TO
#undef I16_TO
#undef U32_TO    
#undef I32_TO
#undef F32_TO
#undef U64_TO
#undef I64_TO
#undef F64_TO
#undef U128_TO
#undef I128_TO
