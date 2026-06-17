#include "hash_map.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lib/linked_list.h"

// Jenkins's hash function
// https://en.wikipedia.org/wiki/Jenkins_hash_function
static uint32_t hash_string(const char *const str) {
  size_t i = 0;
  uint32_t hash = 0;
  while (str[i] != '\0') {
    uint32_t tmp = (uint32_t)str[i++];
    hash += tmp;
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

void hash_map_init(HashMap *map, ValueDestructor value_destructor) {
  map->key_count = 0;
  map->table_size = 4;
  map->value_destructor = value_destructor;
  map->table = calloc(4, sizeof(EntryList));
}

static void hash_map_grow(HashMap *map) {
  if (map->key_count * 4 / map->table_size <= 3) {
    return;
  }

  size_t new_table_size = map->table_size * 2;
  EntryList *new_table = calloc(new_table_size, sizeof(EntryList));

  for (size_t i = 0; i < map->table_size; i++) {
    LL_ITER_CREATE(iter, map->table[i]);

    while (LL_ITER_CURR(iter)) {
      MapEntry *node = LL_ITER_CURR(iter);
      uint32_t hash_key = hash_string(node->key);
      LL_PUSH(new_table[hash_key % new_table_size], *node);

      LL_ITER_NEXT(iter);
    }

    LL_DESTROY(map->table[i]);
  }

  free(map->table);
  map->table = new_table;
  map->table_size = new_table_size;
}

void hash_map_set(HashMap *map, const char *key, void *value) {
  uint32_t key_hash = hash_string(key);

  EntryList *slot = &map->table[key_hash % map->table_size];

  LL_ITER_CREATE(iter, *slot);

  while (LL_ITER_CURR(iter)) {
    MapEntry *node = LL_ITER_CURR(iter);

    if (strcmp(node->key, key) == 0) {
      map->value_destructor(node->value);
      node->value = value;
      return;
    }

    LL_ITER_NEXT(iter);
  }

  map->key_count++;

  hash_map_grow(map);

  slot = &map->table[key_hash % map->table_size];

  LL_PUSH(*slot, ((MapEntry){.key = strdup(key), .value = value}));
}

void hash_map_remove(HashMap *map, const char *key) {
  uint32_t key_hash = hash_string(key);

  EntryList *slot = &map->table[key_hash % map->table_size];

  LL_ITER_CREATE(iter, *slot);

  while (LL_ITER_CURR(iter)) {
    MapEntry *node = LL_ITER_CURR(iter);

    if (strcmp(node->key, key) == 0) {
      free(node->key);
      map->value_destructor(node->value);
      LL_ITER_REMOVE(iter);
      return;
    }

    LL_ITER_NEXT(iter);
  }
}

void *hash_map_get(HashMap *map, const char *key) {
  uint32_t key_hash = hash_string(key);

  EntryList *slot = &map->table[key_hash % map->table_size];

  LL_ITER_CREATE(iter, *slot);

  while (LL_ITER_CURR(iter)) {
    MapEntry *node = LL_ITER_CURR(iter);

    if (strcmp(node->key, key) == 0) {
      return node->value;
    }

    LL_ITER_NEXT(iter);
  }

  return NULL;
}

void hash_map_destroy(HashMap *map) {
  for (size_t i = 0; i < map->table_size; i++) {
    LL_ITER_CREATE(iter, map->table[i]);

    while (LL_ITER_CURR(iter)) {
      MapEntry *node = LL_ITER_CURR(iter);

      free(node->key);
      map->value_destructor(node->value);

      LL_ITER_NEXT(iter);
    }

    LL_DESTROY(map->table[i]);
  }

  free(map->table);
}

void hash_map_iter(HashMap *map, HashMapIterCallback cb) {
  for (size_t i = 0; i < map->table_size; i++) {
    LL_ITER_CREATE(iter, map->table[i]);

    while (LL_ITER_CURR(iter)) {
      MapEntry *node = LL_ITER_CURR(iter);

      cb(node->key, node->value);

      LL_ITER_NEXT(iter);
    }
  }
}
