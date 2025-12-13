#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ENTRIES_PER_BUCKET 4

typedef struct {
  uint8_t count;
  char *key[ENTRIES_PER_BUCKET];
  char *value[ENTRIES_PER_BUCKET];
} hash_bucket;

typedef struct {
  hash_bucket *buckets;
  size_t capacity;
} hash_map;

hash_map hash_map_create(void);

/**
 * @brief get a value given a key in the hash map.
 * @param map the map to use
 * @param key the key of the entry
 * @return returns the associated value or null if no such key exists
 */
const char *const hash_map_get(hash_map *map, const char *const key);

/**
 * @brief set a key to a specified value. in case it isn't obvious, the data
 * passed in is cloned.
 * @param map the map to use.
 * @param key the key of the entry in the map.
 * @param value the value of the entry map.
 */
void hash_map_set(
    hash_map *map, const char *const key, const char *const value
);

/**
 * @brief remove an entry from the map given the key.
 * @param key the key to remove.
 * @return returns 1 if no item was removed, returns 0 otherwise.
 */
int hash_map_unset(hash_map *map, const char *const key);

#endif
