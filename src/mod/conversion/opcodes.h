#pragma once

#include "config.h"

#include "macro_helpers.h"

#ifndef REGISTER_OPCODE

#error "REGISTER_OPCODE() not defined"

#endif

srsvm_word code = 0;

#define CAST_CONVERTER(name,type_from,ctype_from,field_from,type_to,ctype_to,field_to) \
    REGISTER_OPCODE(code++,name,2,4);

#define PARSE_CONVERTER(name,type_to,ctype_to,field_to,fmt) \
    REGISTER_OPCODE(code++,name,2,3);

#define TOSTR_CONVERTER(name,type_from,ctype_from,field_from,fmt) \
    REGISTER_OPCODE(code++,name,2,3);

#define IMPL_U128_PARSERS

#include "conversion_funcs.h"

#undef IMPL_U128_PARSERS

#undef CAST_CONVERTER
#undef PARSE_CONVERTER
#undef TOSTR_CONVERTER
