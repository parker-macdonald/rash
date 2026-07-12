#include "shell_vars/shell_vars.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/hash_map.h"
#include "lib/slice.h"
#include "lib/vector.h"
#include "shell_vars/eval.h"
#include "shell_vars/lexer.h"
#include "shell_vars/token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *SHELL_VAR_KIND_NAMES[SV_COUNT] = {
  [SV_NUMBER] = "number",
  [SV_STRING] = "string",
  [SV_BOOLEAN] = "boolean",
  [SV_NULL] = "null"
};

ShellVar *var_create_string(Buffer string) {
  ShellVar *var = malloc(sizeof(ShellVar));

  var->kind = SV_STRING;
  var->ref_count = 1;
  var->string = string;

  return var;
}

ShellVar *var_create_number(double number) {
  ShellVar *var = malloc(sizeof(ShellVar));

  var->kind = SV_NUMBER;
  var->ref_count = 1;
  var->number = number;

  return var;
}

ShellVar *var_create_boolean(bool boolean) {
  ShellVar *var = malloc(sizeof(ShellVar));

  var->kind = SV_BOOLEAN;
  var->ref_count = 1;
  var->boolean = boolean;

  return var;
}

ShellVar *var_create_null(void) {
  ShellVar *var = malloc(sizeof(ShellVar));

  var->kind = SV_NULL;
  var->ref_count = 1;

  return var;
}

ShellVar *var_aquire(ShellVar *var) {
  var->ref_count++;
  return var;
}

static void var_destroy(ShellVar *var) {
  if (var->kind == SV_STRING) {
    buffer_destroy(&var->string);
  }

  free(var);
}

void var_release(ShellVar *var) {
  var->ref_count--;

  if (var->ref_count == 0) {
    var_destroy(var);
  }
}

ShellVar *var_eval(const char *expr) {
  TokenList list = lex_shell_expr(&slice_using_cstr(expr));

  if (list.length == 0) {
    return NULL;
  }

  ShellVar *var = evaluate_tokens(&list);

  VECTOR_DESTROY(list);

  return var;
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
  ShellVar *var = var_eval(expr);

  if (var == NULL) {
    return NULL;
  }

  Buffer buffer = var_to_string(var);
  var_release(var);

  return buffer_cstr(&buffer);
}

// functions below for messing with the internal hashmap of shellvars to
// identifiers

static HashMap map;

static void var_destructor(void *ptr) {
  ShellVar *var = ptr;
  var_release(var);
}

void var_init(void) {
  hash_map_init(&map, var_destructor);
}

void var_set(const char *key, ShellVar *var) {
  hash_map_set(&map, key, var_aquire(var));
}

ShellVar *var_get(const char *key) {
  ShellVar *var = hash_map_get(&map, key);

  if (var == NULL) {
    return NULL;
  }

  return var_aquire(var);
}

void var_unset(const char *key) { hash_map_remove(&map, key); }

static void print_callback(const char *key, void *ptr) {
  ShellVar *var = ptr;

  printf("{%s}:\t", key);

  switch (var->kind) {
  case SV_NUMBER:
    printf("%g (type: number)\n", var->number);
    break;
  case SV_STRING:
    printf("\"%.*s\" (type: string)\n", (int)var->string.length,
           var->string.char_ptr);
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
