#include <stdlib.h>
#include <string.h>

#include "srsvm/lru.h"

static srsvm_lru_cache_list_node *node_alloc(const char* key, void* data)
{
    srsvm_lru_cache_list_node *node = malloc(sizeof(srsvm_lru_cache_list_node));

    if(node != NULL){
        node->next = NULL;
        node->prev = NULL;

        if(key != NULL){
            strncpy(node->key, key, sizeof(node->key));
        }

        node->data = data;
    }

    return node;
}

srsvm_lru_cache *srsvm_lru_cache_alloc(const bool case_insensitive, const size_t max_capacity, srsvm_lru_cache_evict_func *evict_func, void *evict_arg)
{
    srsvm_lru_cache *cache = malloc(sizeof(srsvm_lru_cache));

    if(cache != NULL){
        if((cache->map = srsvm_string_map_alloc(case_insensitive)) == NULL){
            free(cache);
            cache = NULL;
        } else if((cache->sentinel = node_alloc(NULL, NULL)) == NULL){
            srsvm_string_map_free(cache->map, false);
            free(cache);
            cache = NULL;
        } else {
            cache->sentinel->next = cache->sentinel;
            cache->sentinel->prev = cache->sentinel;

            cache->count = 0;
            cache->max_capacity = max_capacity;

            cache->evict_func = evict_func;
            cache->evict_arg = evict_arg;
        }
    }

    return cache;
}

bool srsvm_lru_cache_contains(const srsvm_lru_cache *cache, const char* key)
{
    if(cache != NULL && key != NULL){
        return srsvm_string_map_contains(cache->map, key);
    } else return false;
}

void srsvm_lru_cache_free(srsvm_lru_cache *cache, const bool free_values)
{
    if(cache != NULL){
        srsvm_string_map_free(cache->map, false);
        
        for(srsvm_lru_cache_list_node *node = cache->sentinel->next,*next = NULL; node != cache->sentinel; node = next)
        {
            next = node->next;

            if(free_values && node->data != NULL){
                free(node->data);
            }

            free(node);
        }

        free(cache->sentinel);
        free(cache);
    }
}

void srsvm_lru_cache_clear(srsvm_lru_cache *cache, const bool free_values){
    if(cache != NULL){
        srsvm_string_map_clear(cache->map, false);
        
        for(srsvm_lru_cache_list_node *node = cache->sentinel->next,*next = NULL; node != cache->sentinel; node = next)
        {
            next = node->next;

            if(free_values && node->data != NULL){
                free(node->data);
            }

            free(node);
        }

        cache->sentinel->next = cache->sentinel;
        cache->sentinel->prev = cache->sentinel;
    }
}

static void promote(srsvm_lru_cache *cache, srsvm_lru_cache_list_node *node)
{
    if(cache != NULL && node != NULL){
        srsvm_lru_cache_list_node *prev = node->prev, *next = node->next;

        prev->next = next;
        next->prev = prev;

        node->next = cache->sentinel->next;
        node->prev = cache->sentinel;

        cache->sentinel->next = node;
    }
}

static void evict(srsvm_lru_cache *cache)
{
    if(cache != NULL){
        srsvm_lru_cache_list_node *node = cache->sentinel->prev;

        if(node != NULL && node != cache->sentinel){
            if(srsvm_string_map_contains(cache->map, node->key)){
                srsvm_string_map_remove(cache->map, node->key, false);
            }

            if(cache->evict_func != NULL)
            {
                cache->evict_func(node->key, node->data, cache->evict_arg);
            }

            cache->sentinel->next = node->prev;
            node->prev->next = cache->sentinel;

            if(cache->count > 0){
                cache->count--;
            }
        }
    }
}

bool srsvm_lru_cache_insert(srsvm_lru_cache *cache, const char* key, void* value)
{
    bool success = false;

    if(cache != NULL && key != NULL){
        if(! srsvm_string_map_contains(cache->map, key)){
            if(cache->count == cache->max_capacity){
                evict(cache);
            }
            
            srsvm_lru_cache_list_node *node = node_alloc(key, value);
        
            if(node != NULL){
                node->next = cache->sentinel->next;
                node->prev = cache->sentinel;

                cache->sentinel->next->prev = node;

                cache->sentinel->next = node;

                cache->count++;

                success = true;
            }
        }
    }

    return success;
}

bool srsvm_lru_cache_remove(srsvm_lru_cache *cache, const char* key, const bool free_value)
{
    bool success = false;

    if(cache != NULL && key != NULL){
        srsvm_lru_cache_list_node *node = srsvm_string_map_lookup(cache->map, key);

        if(node != NULL){
            srsvm_string_map_remove(cache->map, key, false);
        
            node->next->prev = node->prev;
            node->prev->next = node->next;

            if(free_value){
                free(node->data);
            }

            free(node);

            if(cache->count > 0){
                cache->count--;
            }

            success = true;
        }
    }

    return success;
}

void *srsvm_lru_cache_lookup(srsvm_lru_cache *cache, const char* key)
{
    void *data = NULL;

    if(cache != NULL && key != NULL){
        srsvm_lru_cache_list_node *node = srsvm_string_map_lookup(cache->map, key);

        if(node != NULL){
            promote(cache, node);

            return node->data;
        }
    }

    return data;
}
