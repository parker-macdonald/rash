#ifndef HASH_MAP_H
#define HASH_MAP_H

#include "lib/linked_list.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  char *key;
  void *value;
} MapEntry;

typedef LL(MapEntry) EntryList;

typedef void (*ValueDestructor)(void *);

typedef struct {
  size_t key_count;
  size_t table_size;
  ValueDestructor value_destructor;
  EntryList *table;
} HashMap;

void hash_map_init(HashMap *map, ValueDestructor value_destructor);

void hash_map_remove(HashMap *map, const char *key);

void *hash_map_get(HashMap *map, const char *key);

void hash_map_set(HashMap *map, const char *key, void *value);

void hash_map_destroy(HashMap *map);

typedef void (*HashMapIterCallback)(const char *key, void *value);

void hash_map_iter(HashMap *map, HashMapIterCallback cb);

#endif
