#include "shell_vars.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/hash_map.h"
#include "lib/slice.h"
#include "shell_vars/eval.h"
#include "shell_vars/token.h"

#include "lexer.h"

const char *SHELL_VAR_KIND_NAMES[SV_COUNT] = {
  [SV_NUMBER] = "number",
  [SV_STRING] = "string",
  [SV_BOOLEAN] = "boolean",
  [SV_NULL] = "null",
};

static HashMap map;

void var_destroy(ShellVar *var) {
  if (var->kind == SV_STRING) {
    buffer_destroy(&var->string);
  }
}

void var_init(void) {
  hash_map_init(&map, (void (*)(void *))var_destroy);
}

void var_set(const char *key, const ShellVar *var) {
  ShellVar *var_clone = malloc(sizeof(ShellVar));
  memcpy(var_clone, var, sizeof(ShellVar));

  hash_map_set(&map, key, var_clone);
}

ShellVar *var_get(const char *key) {
  return hash_map_get(&map, key);
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
  default:
    unreachable();
  }
}

void var_print(void) {
  hash_map_iter(&map, print_callback);
}

OptionShellVar var_eval(const char *expr) {
  (void)expr;

  TokenList list = lex_shell_expr(&slice_using_cstr(expr));

  return evaluate_tokens(&list);
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
  default:
    unreachable();
  }
}

char *var_eval_to_string(const char *expr) {
  OptionShellVar var = var_eval(expr);

  if (!var.has_value) {
    return NULL;
  }

  Buffer buffer = var_to_string(&var.value);
  var_destroy(&var.value);

  return buffer_cstr(&buffer);
}

ShellVar var_clone(const ShellVar *var) {
  ShellVar clone = *var;

  if (clone.kind == SV_STRING) {
    clone.string = buffer_clone(&clone.string);
  }

  return clone;
}


