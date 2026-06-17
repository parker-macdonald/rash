#include "shell_vars.h"
#include "lib/hash_map.h"

#include <stdio.h>
#include <string.h>

static HashMap map;

void var_init(void) {
  hash_map_init(&map, free);
}

void var_set(const char *const key, const char *const value) {
  hash_map_set(&map, key, strdup(value));
}

const char *var_get(const char *const key) {
  return hash_map_get(&map, key);
}

int var_unset(const char *const key) {
  hash_map_remove(&map, key);

  return 0;
}

static void print_callback(const char *key, void *value) {
  printf("{%s}:\t\"%s\"\n", key, (char *)value);
}

void var_print(void) {
  hash_map_iter(&map, print_callback);
}
