#pragma once

#include <stdbool.h>

#include "srsvm/impl.h"

#define SRSVM_STRING_MAP_KEY_MAX_LEN 512

typedef struct srsvm_string_map_node srsvm_string_map_node;

struct srsvm_string_map_node
{
    srsvm_string_map_node *parent;

    char key[SRSVM_STRING_MAP_KEY_MAX_LEN];
    void *value;

    srsvm_string_map_node *lchild;
    srsvm_string_map_node *rchild;

    unsigned long height;
};

typedef struct
{
    srsvm_lock lock;
    srsvm_string_map_node *root;

    size_t count;

    bool case_insensitive;
} srsvm_string_map;

srsvm_string_map *srsvm_string_map_alloc(const bool case_insensitive);
void srsvm_string_map_free(srsvm_string_map *map, const bool free_vals);

void srsvm_string_map_clear(srsvm_string_map *map, const bool free_vals);

typedef void (*srsvm_string_map_walk_func)(const char*, void*, void*);

void srsvm_string_map_walk(srsvm_string_map *map, srsvm_string_map_walk_func func, void* arg);

bool srsvm_string_map_insert(srsvm_string_map *map, const char* key, void* value);
bool srsvm_string_map_remove(srsvm_string_map *map, const char* key, const bool free_val);

bool srsvm_string_map_contains(srsvm_string_map *map, const char* key);
void* srsvm_string_map_lookup(srsvm_string_map *map, const char* key);
