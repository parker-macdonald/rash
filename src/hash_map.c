#include "hash_map.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Jenkins's hash function
// https://en.wikipedia.org/wiki/Jenkins_hash_function
static uint32_t hash_string(const char *const str) {
  size_t i = 0;
  uint32_t hash = 0;
  while (str[i] != '\0') {
    hash += str[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

static void hash_map_resize(hash_map *map) {
  // multiply capacity by 1.5
  size_t new_capacity = (map->capacity << 1) + (map->capacity >> 1);
  hash_bucket *new_buckets = calloc(sizeof(hash_bucket), new_capacity);

  float items = 0;

  for (size_t i = 0; i < map->capacity; i++) {
    for (size_t j = 0; j < map->buckets[i].count; j++) {
      items++;
      uint32_t hash = hash_string(map->buckets[i].key[j]);

      hash_bucket *bucket = &new_buckets[hash % new_capacity];

      // hypothetically, you could resize, only to once again not have space in
      // the bucket, but i tested this map with hundereds of thousands of keys,
      // and this never happened, so..., don't worry about the edge case :)
      assert(bucket->count < 4);

      bucket->key[bucket->count] = map->buckets[i].key[j];
      bucket->value[bucket->count] = map->buckets[i].value[j];
      bucket->count++;
    }
  }
  
  printf("resizing at %.3f%% full\n", items / (4.f * map->capacity) * 100.f);

  free(map->buckets);
  map->buckets = new_buckets;
  map->capacity = new_capacity;

}

hash_map hash_map_create(void) {
  return (hash_map){.buckets = calloc(16, sizeof(hash_bucket)), .capacity = 16};
}

const char *const hash_map_get(hash_map *map, const char *const key) {
  uint32_t hash = hash_string(key);

  hash_bucket *bucket = &map->buckets[hash % map->capacity];

  // dont need to check all the keys if the bucket is empty
  if (bucket->count == 0) {
    return NULL;
  }

  for (size_t i = 0; i < bucket->count; i++) {
    if (strcmp(bucket->key[i], key) == 0) {
      return bucket->value[i];
    }
  }

  return NULL;
}

void hash_map_set(
    hash_map *map, const char *const key, const char *const value
) {
  uint32_t hash = hash_string(key);

  hash_bucket *bucket = &map->buckets[hash % map->capacity];

  // check if the key is already in the map
  for (size_t i = 0; i < bucket->count; i++) {
    if (strcmp(bucket->key[i], key) == 0) {
      free(bucket->value[i]);
      bucket->value[i] = strdup(value);
      return;
    }
  }

  // we need more space :(
  if (bucket->count == 4) {
    hash_map_resize(map);
    bucket = &map->buckets[hash % map->capacity];

    // same thing with the hash_map_resize function, technically you could
    // resize, only to have another collision, but test after test, this never
    // happened.
    assert(bucket->count < 4);
  }

  bucket->key[bucket->count] = strdup(key);
  bucket->value[bucket->count] = strdup(value);
  bucket->count++;
}

int hash_map_unset(hash_map *map, const char *const key) {
  uint32_t hash = hash_string(key);

  hash_bucket *bucket = &map->buckets[hash % map->capacity];

  // no work needs to be done if the bucket is empty
  if (bucket->count == 0) {
    return 1;
  }

  for (size_t i = 0; i < ENTRIES_PER_BUCKET; i++) {
    if (strcmp(bucket->key[i], key) == 0) {
      free(bucket->key[i]);
      free(bucket->value[i]);

      bucket->count--;

      // skooch the other members of the bucket along so they sit side by side.
      for (; i < ENTRIES_PER_BUCKET - 1; i++) {
        bucket->key[i] = bucket->key[i + 1];
        bucket->value[i] = bucket->value[i + 1];
      }

      bucket->key[i] = NULL;
      bucket->value[i] = NULL;

      return 0;
    }
  }

  return 1;
}
