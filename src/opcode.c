#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "impl.h"
#include "memory.h"
#include "mmu.h"
#include "opcode.h"
#include "vm.h"

srsvm_opcode_map *srsvm_opcode_map_alloc(void)
{
    srsvm_opcode_map *map;

    map = malloc(sizeof(srsvm_opcode_map));

    if(map != NULL){
        srsvm_lock_initialize(&map->lock);
        map->by_name_root = NULL;
        map->by_code_root = NULL;
        map->count = 0;
    }

    return map;
}

void free_node(srsvm_opcode_map_node *node)
{
    if(node != NULL){
        if(node->code_lchild != NULL){
            free_node(node->code_lchild);
        }

        if(node->code_rchild != NULL){
            free_node(node->code_rchild);
        }

        free(node);
    }
}

void srsvm_opcode_map_free(srsvm_opcode_map *map)
{
    if(map != NULL){
        srsvm_lock_destroy(&map->lock);

        free_node(map->by_code_root);

        free(map);
    }
}

static srsvm_opcode_map_node *search_by_name(const srsvm_opcode_map *map, const char* opcode_name)
{
    srsvm_opcode_map_node *node = map->by_name_root;
    int cmp_result;

    while(node != NULL){
        cmp_result = strncmp(node->opcode->name, opcode_name, sizeof(node->opcode->name));
        
        if(cmp_result == 0){
            break;
        } else if(cmp_result < 0){
            node = node->name_lchild;
        } else {
            node = node->name_rchild;
        }
    }

    return NULL;
}

static srsvm_opcode_map_node *search_by_code(const srsvm_opcode_map *map, const srsvm_word opcode_code)
{
    srsvm_opcode_map_node *node = map->by_code_root;
    
    while(node != NULL){
        if(node->opcode->code == opcode_code){
            return node;
        } else if(node->opcode->code == opcode_code){
            node = node->code_lchild;
        } else {
            node = node->code_rchild;
        }
    }

    return NULL;
}

bool opcode_name_exists(const srsvm_opcode_map *map, const char* opcode_name)
{
    return search_by_name(map, opcode_name) != NULL;
}

bool opcode_code_exists(const srsvm_opcode_map *map, const srsvm_word opcode_code)
{
    return search_by_code(map, opcode_code) != NULL;
}

srsvm_opcode *opcode_lookup_by_name(const srsvm_opcode_map *map, const char* opcode_name)
{
    srsvm_opcode_map_node *node = search_by_name(map, opcode_name);

    if(node == NULL){
        return NULL;
    } else {
        return node->opcode;
    }
}

srsvm_opcode *opcode_lookup_by_code(const srsvm_opcode_map *map, const srsvm_word opcode_code)
{
    srsvm_opcode_map_node *node = search_by_code(map, opcode_code);

    if(node == NULL){
        return NULL;
    } else {
        return node->opcode;
    }
}

bool opcode_map_insert(srsvm_opcode_map *map, srsvm_opcode* opcode)
{
    bool success = false;

    if(search_by_code(map, opcode->code) != NULL){

    } else if(strlen(opcode->name) > 0 && search_by_name(map, opcode->name) != NULL){

    } else {
        srsvm_opcode_map_node *node = malloc(sizeof(srsvm_opcode_map_node));
        
        if(node != NULL){
            node->name_parent = NULL;
            node->code_parent = NULL;
            node->opcode = opcode;
            node->name_lchild = NULL;
            node->name_rchild = NULL;
            node->code_lchild = NULL;
            node->code_rchild = NULL;

            if(strlen(opcode->name) > 0){
                if(map->by_name_root == NULL){
                    map->by_name_root = node;
                } else {
                    srsvm_opcode_map_node *parent_node = map->by_name_root;

                    while(parent_node != NULL){
                        int cmp_result = strncmp(opcode->name, node->opcode->name, sizeof(opcode->name));

                        if(cmp_result < 0){
                            if(parent_node->name_lchild == NULL){
                                parent_node->name_lchild = node;
                                break;
                            } else parent_node = parent_node->name_lchild;
                        } else {
                            if(parent_node->name_rchild == NULL){
                                parent_node->name_rchild = node;
                                break;
                            } else parent_node = parent_node->name_rchild;
                        }
                    }

                    node->name_parent = parent_node;
                }
            }

            if(map->by_code_root == NULL){
                map->by_code_root = node;
            } else {
                srsvm_opcode_map_node *parent_node = map->by_code_root;

                while(parent_node != NULL){
                    if(opcode->code < parent_node->opcode->code){
                        if(parent_node->code_lchild == NULL){
                            parent_node->code_lchild = node;
                            break;
                        } else parent_node = parent_node->code_lchild;
                    } else {
                        if(parent_node->code_rchild == NULL){
                            parent_node->code_rchild = node;
                            break;
                        } else parent_node = parent_node->code_rchild;
                    }
                }

                node->code_parent = parent_node;
            }

            map->count++;

            success = true;
        }
    }

    return success;
}

bool load_instruction(srsvm_vm *vm, const srsvm_ptr addr, srsvm_instruction *instruction)
{
    bool success = false;

    instruction->opcode = 0;
    memset(&instruction->argv, 0, sizeof(instruction->argv));
    instruction->argc = 0;

    srsvm_memory_segment *seg = srsvm_mmu_locate(vm->mem_root, addr);
    if(seg != NULL){
        if(seg->executable){
            if(! srsvm_mmu_load(seg, addr, (srsvm_word) sizeof(instruction->opcode), &instruction->opcode)){

            } else {
                instruction->argc = OPCODE_ARGC(instruction->opcode);
                instruction->opcode &= ~OPCODE_ARGC_MASK;

                if(! srsvm_mmu_load(seg, addr + (srsvm_word) (sizeof(instruction->opcode)), (srsvm_word) (instruction->argc * sizeof(srsvm_word)), &instruction->argv)){

                } else {
                    success = true;
                }
            }
        }
    }

    return success;
}
