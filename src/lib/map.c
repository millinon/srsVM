#include <stdlib.h>
#include <string.h>

#include "srsvm/debug.h"
#include "srsvm/map.h"

srsvm_string_map_node * srsvm_string_map_node_alloc(const char* key, void* value)
{
    srsvm_string_map_node *node = malloc(sizeof(srsvm_string_map_node));

    if(node != NULL){
        node->parent = NULL;

        memset(node->key, 0, sizeof(node->key));
		srsvm_strncpy(node->key, key, sizeof(node->key) - 1);
        node->value = value;

        node->lchild = NULL;
        node->rchild = NULL;

        node->height = 0;
    }

    return node;
}

static void srsvm_string_map_node_free(srsvm_string_map_node *node, const bool free_val)
{
    if(node != NULL){
        if(free_val){
            free(node->value);
        }

        if(node->lchild != NULL){
            srsvm_string_map_node_free(node->lchild, free_val);
        }
        
        if(node->rchild != NULL){
            srsvm_string_map_node_free(node->rchild, free_val);
        }

        free(node);
    }
}

srsvm_string_map *srsvm_string_map_alloc(const bool case_insensitive)
{
    srsvm_string_map *map = malloc(sizeof(srsvm_string_map));

    if(map != NULL){
        map->root = NULL;
        map->count = 0;
        map->case_insensitive = case_insensitive;

        if(! srsvm_lock_initialize(&map->lock)){
            free(map);
            map = NULL;    
        }
    }

    return map;
}

void srsvm_string_map_free(srsvm_string_map *map, const bool free_vals)
{
    if(map != NULL){
        if(map->root != NULL){
            srsvm_string_map_node_free(map->root, free_vals);
        }

        srsvm_lock_destroy(&map->lock);
        free(map);
    }
}

void srsvm_string_map_clear(srsvm_string_map *map, const bool free_vals){
    if(map != NULL){
        if(map->root != NULL){
            srsvm_string_map_node_free(map->root, free_vals);
        }

        map->root = NULL;
    }
}

static unsigned long reassign_height(srsvm_string_map_node *node)
{
    unsigned long lchild_height = 0;
    if(node->lchild != NULL){
        lchild_height = reassign_height(node->lchild) + 1;
    }

    unsigned long rchild_height = 0;
    if(node->rchild != NULL){
        rchild_height = reassign_height(node->rchild) + 1;
    }

    if(lchild_height > rchild_height){
        node->height = rchild_height;
    } else {
        node->height = lchild_height;
    }

    return node->height;
}

bool srsvm_string_map_remove(srsvm_string_map *map, const char* key, const bool free_val)
{
    bool success = false;
    srsvm_string_map_node *node = NULL;

    if(map != NULL && key != NULL){
        if(srsvm_lock_acquire(&map->lock, 0)){

            srsvm_string_map_node *parent_node = map->root;

            while(parent_node != NULL){
                int cmp_result = strncmp(key, parent_node->key, sizeof(parent_node->key));

                if(cmp_result < 0){
                    parent_node = parent_node->lchild;
                } else if(cmp_result > 0){
                    parent_node = parent_node->rchild;
                } else {
                    node = parent_node;

                    break;
                }
            }

            if(node != NULL){
                parent_node = node->parent;

                srsvm_string_map_node *new_parent = NULL;

                if(node->lchild == NULL && node->rchild == NULL){
                    new_parent = NULL;
                } else if(node->lchild == NULL){
                    new_parent = node->rchild;
                } else if(node->rchild == NULL){
                    new_parent = node->lchild;
                } else {
                    srsvm_string_map_node *furthest = NULL;

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
        
                srsvm_string_map_node_free(node, free_val);

                map->count--;

                success = true;
            }
            
            srsvm_lock_release(&map->lock);
        }
    }

    return success;
}

bool srsvm_string_map_insert(srsvm_string_map *map, const char* key, void* value)
{
    bool success = false;

    srsvm_string_map_node *node = NULL;

    if(map != NULL && key != NULL && !srsvm_string_map_contains(map, key)){
        if(! srsvm_lock_acquire(&map->lock, 0)){
            return false;
        }

        node = srsvm_string_map_node_alloc(key, value);

        if(node == NULL){
            goto error_cleanup;
        }

        if(map->root == NULL){
            map->root = node;
            map->count = 1;
            success = true;
        } else {
            srsvm_string_map_node *parent_node = map->root;

            while(parent_node != NULL){
                int cmp_result;

                if(map->case_insensitive){
                    cmp_result = srsvm_strcasecmp(node->key, parent_node->key);
                } else {
                    cmp_result = strcmp(node->key, parent_node->key);
                }

                if(cmp_result < 0){
                    if(parent_node->lchild == NULL){
                        parent_node->lchild = node;
                        node->parent = parent_node;
                        map->count++;

                        success = true;

                        break;
                    } else parent_node = parent_node->lchild;
                } else if(cmp_result > 0){
                    if(parent_node->rchild == NULL){
                        parent_node->rchild = node;
                        node->parent = parent_node;
                        map->count++;

                        success = true;

                        break;
                    } else parent_node = parent_node->rchild;
                } else break;
            }
        }
    }

    srsvm_lock_release(&map->lock);

    if(! success){
        dbg_printf("failed to insert %s\n", key);
    }

    return success;

error_cleanup:
    if(node != NULL){
        srsvm_string_map_node_free(node, false);
    }
    if(map != NULL){
        srsvm_lock_release(&map->lock);
    }

    return false;
}

static srsvm_string_map_node* search(srsvm_string_map *map, const char* key)
{
    srsvm_string_map_node *node = NULL;

    if(map != NULL && key != NULL){
        if(! srsvm_lock_acquire(&map->lock, 0)){
            return NULL;
        }

        srsvm_string_map_node *parent_node = map->root;

        while(parent_node != NULL){
            int cmp_result = strncmp(key, parent_node->key, sizeof(parent_node->key));

            if(cmp_result < 0){
                parent_node = parent_node->lchild;
            } else if(cmp_result > 0){
                parent_node = parent_node->rchild;
            } else {
                node = parent_node;

                break;
            }
        }
        srsvm_lock_release(&map->lock);
    }

    return node;
}

bool srsvm_string_map_contains(srsvm_string_map *map, const char* key)
{
    return search(map, key) != NULL;    
}

void *srsvm_string_map_lookup(srsvm_string_map *map, const char* key)
{
    srsvm_string_map_node *node = search(map, key);

    if(node != NULL){
        return node->value;
    } else return NULL;
}

static void inorder_walk(srsvm_string_map_node *node, srsvm_string_map_walk_func func, void* arg)
{
    if(node != NULL){
        if(node->lchild != NULL){
            inorder_walk(node->lchild, func, arg);
        }

        func(node->key, node->value, arg);

        if(node->rchild != NULL){
            inorder_walk(node->rchild, func, arg);
        }
    }
}

void srsvm_string_map_walk(srsvm_string_map *map, srsvm_string_map_walk_func func, void* arg)
{
    if(map != NULL && map->root != NULL && func != NULL){
        inorder_walk(map->root, func, arg);
    }
}
