#if !defined(WORD_SIZE)

#error "WORD_SIZE not set"

#endif

#include <stdlib.h>
#include <string.h>

#include "srsvm/module.h"

#include "config.h"

#include "macro_helpers.h"
#include "mod_math.h"

static bool register_opcode(srsvm_module_opcode_loader loader, srsvm_word code, const char* name, const unsigned short a_min, const unsigned short a_max, srsvm_opcode_func *func, void* arg)
{
    bool success = false;

    srsvm_opcode *opcode = malloc(sizeof(srsvm_opcode));

    if(opcode != NULL){
        opcode->code = code;
        memset(opcode->name, 0, sizeof(opcode->name));
        if(strlen(name) > 0){
            strncpy(opcode->name, name, sizeof(opcode->name) - 1);
        }
        opcode->argc_min = a_min;
        opcode->argc_max = a_max;
        opcode->func = func;

        if(loader(arg, opcode)){
            success = true;
        }
    }

    return success;
}


SRSVM_EXPORT bool EVAL2(srsvm_enumerate_opcodes,WORD_SIZE)(srsvm_module_opcode_loader loader, void* arg)
{
    bool success = true;

#define REGISTER_OPCODE(c,n,a_min,a_max) do { \
    if(! register_opcode(loader, c, STR(n), a_min, a_max, EVAL3(math,n,WORD_SIZE), arg)){ \
        success = false; \
    } } while(0) ;

#include "opcodes.h"

    return success;
}
