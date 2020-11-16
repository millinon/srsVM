#include <stdio.h>
#include <string.h>

#include "srsvm/debug.h"
#include "srsvm/opcode.h"
#include "srsvm/program.h"

bool srsvm_debug_mode;

int main(){
    srsvm_debug_mode = true;

    srsvm_word static_program[] = {
        (OPCODE_MK_ARGC(2) | 9), 3, 0,
        (OPCODE_MK_ARGC(2) | 9), 2, 2,
        (OPCODE_MK_ARGC(1) | 1337), 3,
        (OPCODE_MK_ARGC(1) | 30), 1,
        (OPCODE_MK_ARGC(3) | 40), 0, 1, 2,
        (OPCODE_MK_ARGC(2) | 9), 4, 3,
        (OPCODE_MK_ARGC(2) | 6), 4, 0,
        (OPCODE_MK_ARGC(2) | 9), 4, 4,
        (OPCODE_MK_ARGC(1) | 4), 4,
        (OPCODE_MK_ARGC(2) | 9), 3, 1,
        (OPCODE_MK_ARGC(1) | 1337), 3,
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
    reg->next = srsvm_program_register_alloc();
    reg = reg->next;
    strcpy(reg->name, "ACC");
    reg->index = 1;
    reg->next = srsvm_program_register_alloc();
    reg = reg->next;
    strcpy(reg->name, "TARG");
    reg->index = 2;
    reg->next = srsvm_program_register_alloc();
    reg = reg->next;
    strcpy(reg->name, "STR");
    reg->index = 3;
    reg->next = srsvm_program_register_alloc();
    reg = reg->next;
    strcpy(reg->name, "OFF");
    reg->index = 4;
    program->num_registers = 5;

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
    c->next = srsvm_program_const_alloc();
    c = c->next;
    c->const_val.type = STR;
    c->const_val.str = "All done.";
    c->const_val.str_len = strlen("All done.");
    c->const_slot = 1;
    c->next = srsvm_program_const_alloc();
    c = c->next;
    c->const_val.type = WORD;
    c->const_val.word = 10;
    c->const_slot = 2;
    c->next = srsvm_program_const_alloc();
    c = c->next;
    c->const_val.type = PTR_OFFSET;
    c->const_val.ptr_offset = 8 * (srsvm_word) sizeof(srsvm_word);
    c->const_slot = 3;
    c->next = srsvm_program_const_alloc();
    c = c->next;
    c->const_val.type = PTR_OFFSET;
    c->const_val.ptr_offset = -17 * (srsvm_word) sizeof(srsvm_word);
    c->const_slot = 4;
    program->num_constants = 5;


    if(! srsvm_program_serialize("hello_world.svm", program)){
        fprintf(stderr, "Failed to serialize to hello_world.svm\n");
    } else {
        printf("Serialized to hello_world.svm\n");
    }
    
    srsvm_program_free(program);
}
