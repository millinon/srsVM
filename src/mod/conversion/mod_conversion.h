#pragma once

#include "config.h"

#if !defined(WORD_SIZE)

#error "WORD_SIZE not defined"

#endif

#define CAST_CONVERTER(name,type_from,ctype_from,field_from,type_to,ctype_to,field_to) \
    void EVAL3(conversion,name,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]);
#define PARSE_CONVERTER(name,type_to,ctype_to,field_to,fmt) \
    void EVAL3(conversion,name,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]);
#define TOSTR_CONVERTER(name,type_from,ctype_from,field_from,fmt) \
    void EVAL3(conversion,name,WORD_SIZE)(srsvm_vm *vm, srsvm_thread *thread, const srsvm_word argc, const srsvm_word argv[]);

#define IMPL_U128_PARSERS
#include "conversion_funcs.h"
#undef IMPL_U128_PARSERS

#undef CAST_CONVERTER
#undef PARSE_CONVERTER
#undef TOSTR_CONVERTER
