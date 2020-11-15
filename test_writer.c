#include <stdio.h>
#include <string.h>

#include "srsvm/debug.h"
#include "srsvm/opcode.h"
#include "srsvm/program.h"

bool srsvm_debug_mode;

int main(){
    srsvm_debug_mode = true;

    srsvm_word static_program[] = {
        (OPCODE_MK_ARGC(2) | 7), 0, 0, 
        (OPCODE_MK_ARGC(1) | 1337), 0,
        (OPCODE_MK_ARGC(0) | 1),
    };

    srsvm_program *program = srsvm_program_alloc();
    
    srsvm_program_metadata *metadata = srsvm_program_metadata_alloc();
    program->metadata = metadata;
    metadata->word_size = WORD_SIZE;
    metadata->entry_point = 0x1000;

    srsvm_register_specification *reg = srsvm_program_register_alloc();
    program->registers = reg;
    strcpy(reg->name, "R0");
    reg->index = 0;
    program->num_registers = 1;

    srsvm_literal_memory_specification *lmem = srsvm_program_lmem_alloc();
    program->literal_memory = lmem;
    lmem->start_address = 0x1000;
    lmem->size = sizeof(static_program);
    lmem->is_compressed = true;

    size_t compressed_size;
    lmem->data = srsvm_zlib_deflate(static_program, &compressed_size, (size_t) lmem->size);
    lmem->compressed_size = (srsvm_word) compressed_size;
    
    lmem->readable = true;
    lmem->executable = true;
    lmem->locked = true;
    program->num_lmem_segments = 1;

    srsvm_constant_specification *c = srsvm_program_const_alloc();
    program->constants = c;
    c->const_val.type = STR;
    c->const_val.str = "Hello, world!";
    c->const_val.str_len = strlen("Hello, world!");
    c->const_slot = 0;
    program->num_constants = 1;

    if(! srsvm_program_serialize("hello_world.svm", program)){
        fprintf(stderr, "Failed to serialize to hello_world.svm\n");
    } else {
        printf("Serialized to hello_world.svm\n");
    }

    srsvm_program_free(program);
}
