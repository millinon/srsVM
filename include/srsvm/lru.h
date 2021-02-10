#pragma once

#include "srsvm/map.h"

typedef void (srsvm_lru_cache_evict_func)(const char* evicted_key, void* evicted_arg, void* state_arg);

typedef struct srsvm_lru_cache_list_node srsvm_lru_cache_list_node;

struct srsvm_lru_cache_list_node
{
    srsvm_lru_cache_list_node *next;
    srsvm_lru_cache_list_node *prev;

    char key[SRSVM_STRING_MAP_KEY_MAX_LEN];

    void* data;
};

typedef struct
{
    srsvm_lru_cache_list_node *sentinel;
    
    size_t count;
    size_t max_capacity;

    srsvm_lru_cache_evict_func *evict_func;
    void *evict_arg;

    srsvm_string_map *map;
} srsvm_lru_cache;

srsvm_lru_cache *srsvm_lru_cache_alloc(const bool case_insensitive, const size_t max_capacity, srsvm_lru_cache_evict_func *evict_func, void *evict_arg);
void srsvm_lru_cache_free(srsvm_lru_cache *cache, const bool free_values);

void srsvm_lru_cache_clear(srsvm_lru_cache *cache, const bool free_values);

bool srsvm_lru_cache_contains(const srsvm_lru_cache *cache, const char* key);

bool srsvm_lru_cache_insert(srsvm_lru_cache *cache, const char* key, void* value);
bool srsvm_lru_cache_remove(srsvm_lru_cache *cache, const char* key, const bool free_value);

void *srsvm_lru_cache_lookup(srsvm_lru_cache *cache, const char* key);
