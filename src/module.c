#include <stdlib.h>
#include <string.h>

#include "module.h"

static bool load_opcode(void* arg, srsvm_opcode* opcode)
{
    srsvm_module *mod = arg;

    bool success = false;

    if(strlen(opcode->name) > 0 && opcode_lookup_by_name(mod->opcode_map, opcode->name) != NULL){

    } else if(opcode_lookup_by_code(mod->opcode_map, opcode->code) != NULL){

    } else if(! opcode_map_insert(mod->opcode_map, opcode)) {

    } else {
        return true;
    }

    return success;
}

srsvm_module *srsvm_module_alloc(const char* name, const char* filename, srsvm_word id)
{
    srsvm_module *mod = NULL;

    mod = malloc(sizeof(srsvm_module));

    if(mod != NULL){
        mod->ref_count = 1;
        mod->id = id;

        if(strlen(name) < SRSVM_MODULE_MAX_NAME_LEN){
           strncpy(mod->name, name, sizeof(mod->name));

           if(srsvm_native_module_load(&mod->handle, filename)){
                mod->opcode_map = srsvm_opcode_map_alloc();

                if(mod->opcode_map == NULL){
                    goto error_cleanup;
                } else if(! srsvm_native_module_load_opcodes(&mod->handle, load_opcode, mod)){
                    goto error_cleanup;
                }
           } else {
               goto error_cleanup;
           }
        } else {
            goto error_cleanup;
        }
    }

    return mod;

error_cleanup:
    srsvm_module_free(mod);

    return NULL;
}

void srsvm_module_free(srsvm_module *mod)
{
    if(mod != NULL){
        if(mod->opcode_map != NULL) srsvm_opcode_map_free(mod->opcode_map);
        srsvm_native_module_unload(mod->handle);
        free(mod);
    }
}

srsvm_module_map *srsvm_module_map_alloc(void)
{
    srsvm_module_map *map = NULL;

    map = malloc(sizeof(srsvm_module_map));

    if(map != NULL){
        srsvm_lock_initialize(&map->lock);
        map->root = NULL;
        map->count = 0;
    }

    return map;
}

static void free_node(srsvm_module_map_node *node)
{
    if(node != NULL){
        if(node->lchild != NULL){
            free_node(node->lchild);
        }

        if(node->rchild != NULL){
            free_node(node->rchild);
        }

        free(node);
    }
}

void srsvm_module_map_free(srsvm_module_map *map)
{
    if(map != NULL){
        srsvm_lock_destroy(&map->lock);

        free_node(map->root);

        free(map);
    }
}

static srsvm_module_map_node *search_by_name(const srsvm_module_map *map, const char* module_name)
{
    srsvm_module_map_node *node = map->root;

    int cmp_result;

    while(node != NULL){
        cmp_result = strncmp(node->mod->name, module_name, SRSVM_MODULE_MAX_NAME_LEN);

        if(cmp_result == 0){
            break;
        } else if(cmp_result < 0){
            node = node->lchild;
        } else {
            node = node->rchild;
        }
    }

    return node;
}

bool srsvm_module_exists(const srsvm_module_map *map, const char* module_name)
{
    return search_by_name(map, module_name) != NULL;
}

srsvm_module *srsvm_module_lookup(const srsvm_module_map *map, const char* module_name)
{
    srsvm_module_map_node *node = search_by_name(map, module_name);

    if(node == NULL){
        return NULL;
    } else {
        return node->mod;
    }
}

bool srsvm_module_map_insert(srsvm_module_map *map, srsvm_module *module)
{
    bool success = false;

    if(search_by_name(map, module->name) != NULL){

    } else {
        srsvm_module_map_node *node = malloc(sizeof(srsvm_opcode_map_node));

        if(node != NULL){
            node->parent = NULL;
            node->mod = module;
            node->lchild = NULL;
            node->rchild = NULL;
            
            node->height = 0;

            if(map->root == NULL){
                map->root = node;
            } else {
                srsvm_module_map_node *parent_node = map->root;

                while(parent_node != NULL){
                    int cmp_result = strncmp(module->name, node->mod->name, SRSVM_MODULE_MAX_NAME_LEN);

                    if(cmp_result < 0){
                        if(parent_node->lchild == NULL){
                            parent_node->lchild = node;
                            break;
                        } else parent_node = parent_node->lchild;
                    } else {
                        if(parent_node->rchild == NULL){
                            parent_node->rchild = node;
                            break;
                        } else parent_node = parent_node->rchild;
                    }
                }

                node->parent = parent_node;

                unsigned height = 0;
                while(parent_node != NULL){
                    if(++height > parent_node->height){
                        parent_node->height = height;
                        parent_node = parent_node->parent;
                    } else break;
                }
            }

            map->count++;

            success = true;
        }
    }


    return success;
}

static unsigned reassign_height(srsvm_module_map_node *node)
{
    unsigned lchild_height = 0;
    if(node->lchild != NULL){
        lchild_height = reassign_height(node->lchild) + 1;
    }

    unsigned rchild_height = 0;
    if(node->rchild != NULL){
        rchild_height = reassign_height(node->rchild) + 1;
    }

    if(lchild_height > rchild_height){
        node->height = lchild_height;
    } else {
        node->height = rchild_height;
    }

    return node->height;
}

bool srsvm_module_map_remove(srsvm_module_map *map, const char* module_name)
{
    bool success = false;

    srsvm_module_map_node *node = search_by_name(map, module_name), *new_parent;

    if(node != NULL){
        if(node->lchild == NULL && node->rchild == NULL){
            new_parent = NULL;
        } else if(node->lchild == NULL){
            new_parent = node->rchild;
        } else if(node->rchild == NULL){
            new_parent = node->lchild;
        } else {
            srsvm_module_map_node *furthest;

            if(node->lchild->height < node->rchild->height){
                new_parent = node->lchild;
                furthest = new_parent;

                while(furthest->rchild != NULL){
                    furthest = furthest->rchild;
                }

                furthest->rchild = node->rchild;
                node->rchild->parent = furthest;

                node->rchild = NULL;
            } else {
                new_parent = node->rchild;
                furthest = new_parent;

                while(furthest->lchild != NULL){
                    furthest = furthest->lchild;
                }

                furthest->lchild = node->lchild;
                node->lchild->parent = furthest;

                node->lchild = NULL;
            }
        }

        if(new_parent != NULL){
            new_parent->parent = node->parent;
        }

        if(node->parent != NULL){
            if(node == node->parent->lchild){
                node->parent->lchild = new_parent;
            } else {
                node->parent->rchild = new_parent;
            }
        } else if(node == map->root){
            map->root = new_parent;
        }

        if(map->root != NULL){
            reassign_height(map->root);
        }

        free(node);

        map->count--;

        success = true;
    }

    return success;
}
