#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_map.h"

void rand_string(char* buf) {
  size_t len = rand() % 255;

  size_t i;
  for (i = 0; i < len; i++) {
    buf[i] = rand() % 94 + 32;
  }

  buf[i] = '\0';
}

int main(void) {
  hash_map map = hash_map_create();
  srand(0);

  while (1) {
    char key[256];
    char value[256];

    rand_string(key);
    rand_string(value);

    hash_map_set(&map, key, value);
    if (strcmp(value, hash_map_get(&map, key)) != 0) {
      hash_map_get(&map, key);
      assert(0);
    }

    if (rand() % 5) {
      hash_map_unset(&map, key);
    }
  }

  return 0;
}