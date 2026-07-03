#include "shell_vars.h"

#include <stdio.h>
#include <string.h>

#include "lib/buffer.h"
#include "lib/hash_map.h"

static HashMap map;

static void var_destroy(void *ptr) {
  ShellVar *var = ptr;

  if (var->kind == SV_STRING) {
    buffer_destroy(&var->string);
  }
}

void var_init(void) {
  hash_map_init(&map, var_destroy);
}

void var_set(const char *key, const ShellVar *var) {
  ShellVar *var_clone = malloc(sizeof(ShellVar));
  memcpy(var_clone, var, sizeof(ShellVar));

  hash_map_set(&map, key, var_clone);
}

int var_unset(const char *const key) {
  hash_map_remove(&map, key);

  return 0;
}

static void print_callback(const char *key, void *ptr) {
  ShellVar *var = ptr;
  
  printf("{%s}:\t", key);

  switch (var->kind) {
  case SV_NUMBER:
    printf("%f (type: number)\n", var->number);
    break;
  case SV_STRING:
    printf("\"%.*s\" (type: string)\n", (int)var->string.length, var->string.char_ptr);
    break;
  case SV_BOOLEAN:
    printf("%s (type: boolean)\n", var->boolean ? "true" : "false");
    break;
  case SV_NULL:
    printf("null (type: null)\n");
    break;
  }
}

void var_print(void) {
  hash_map_iter(&map, print_callback);
}

ShellVar var_eval(const char *expr) {
  (void)expr;

  return (ShellVar){.kind = SV_NULL};
}

Buffer var_to_string(const ShellVar *var) {
  switch (var->kind) {
  case SV_NUMBER:
    return buffer_from_format("%g", var->number);
  case SV_STRING:
    return buffer_clone(&var->string);
  case SV_BOOLEAN:
    return buffer_from_format("%s", var->boolean ? "true" : "false");
  case SV_NULL:
    return buffer_from_cstr("null");
  }
}

char *var_eval_to_string(const char *expr) {
  ShellVar var = var_eval(expr);

  Buffer buffer = var_to_string(&var);

  var_destroy(&var);

  return buffer_cstr(&buffer);
}
