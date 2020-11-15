#include <stdlib.h>

#include "srsvm/constant.h"
#include "srsvm/opcode-helpers.h"

srsvm_constant_value *srsvm_const_alloc(srsvm_value_type type)
{
    srsvm_constant_value *c = malloc(sizeof(srsvm_constant_value));

    if(c != NULL){
        c->type = type;
    }

    return c;
}

void srsvm_const_free(srsvm_constant_value *c)
{
    if(c != NULL){
        free(c);
    }
}

bool srsvm_const_load(srsvm_register *dest_reg, srsvm_constant_value *val, const srsvm_word offset)
{
    bool success = false;

    if(dest_reg != NULL && val != NULL){
        switch(val->type){
            #define LOADER(name,flag) \
                case flag: \
                    success = load_##name(dest_reg, val->name, offset); \
                    break;
            
            LOADER(word, WORD);
            LOADER(ptr, PTR);
            LOADER(ptr_offset, PTR_OFFSET);
            LOADER(bit, BIT);
            LOADER(u8, U8);
            LOADER(i8, I8);
            LOADER(u16, U16);
            LOADER(i16, I16);
#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
            LOADER(u32, U32);
            LOADER(i32, I32);
            LOADER(f32, F32);
#endif
#if WORD_SIZE == 64 || WORD_SIZE == 128
            LOADER(u64, U64);
            LOADER(i64, I64);
            LOADER(f64, F64);
#endif
#if WORD_SIZE == 128
            LOADER(u128, U128);
            LOADER(i128, I128);
#endif
#undef LOADER
            case STR:
                success = load_str(dest_reg, val->str, val->str_len);
                break;
            default:
                break;
        }
    }

    return success;
}
