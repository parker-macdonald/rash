#include "shell_vars/shell_vars.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lib/buffer.h"
#include "lib/hash_map.h"
#include "lib/parse.h"
#include "lib/slice.h"
#include "lib/sys.h"
#include "shell_vars/eval.h"
#include "shell_vars/functions.h"
#include "shell_vars/lexer.h"
#include "shell_vars/token.h"

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

ShellVar *var_create_function(ShellVar *(*func)(ShellVar *const *args), unsigned arg_count) {
  ShellVar *var = malloc(sizeof(ShellVar));

  var->kind = SV_FUNCTION;
  var->ref_count = 1;

  var->function.arg_count = arg_count;
  var->function.func = func;

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
  Slice source = slice_from_cstr(expr);
  TokenList list = lex_shell_expr(&source);

  if (list.length == 0) {
    return NULL;
  }

  ShellVar *var = evaluate_tokens(&list);

  token_list_free(&list);

  return var;
}

Buffer var_to_string(const ShellVar *var) {
  switch (var->kind) {
    case SV_NUMBER:
      return buffer_from_format("%g", var->number);
    case SV_STRING:
      return buffer_clone(&var->string);
    case SV_BOOLEAN:
      return buffer_from_cstr(var->boolean ? "true" : "false");
    case SV_NULL:
      return buffer_from_cstr("null");
    case SV_FUNCTION:
      return buffer_from_cstr("function");
  }

  return (Buffer){0};
}

ShellVar *var_cast_to_string(const ShellVar *var) {
  return var_create_string(var_to_string(var));
}

bool var_to_boolean(const ShellVar *var) {
  switch (var->kind) {
    case SV_NUMBER:
      return var->number != 0;
      break;
    case SV_STRING:
      return var->string.length != 0;
      break;
    case SV_BOOLEAN:
      return var->boolean;
      break;
    case SV_NULL:
      return false;
      break;
    case SV_FUNCTION:
      return true;
  }
}

const char *var_kind_to_string(ShellVarKind kind) {
  switch (kind) {
    case SV_NUMBER:
      return "number";
    case SV_STRING:
      return "string";
    case SV_BOOLEAN:
      return "boolean";
    case SV_NULL:
      return "null";
    case SV_FUNCTION:
      return "function";
    }

  return NULL;
}

ShellVar *var_cast_to_boolean(const ShellVar *var) {
  return var_create_boolean(var_to_boolean(var));
}

ShellVar *var_cast_to_number(const ShellVar *var) {
  double number;

  switch (var->kind) {
    case SV_NUMBER:
      number = var->number;
      break;
    case SV_STRING: {
      Buffer copy = buffer_clone(&var->string);
      OptionDouble parsed = parse_double(buffer_cstr(&copy));
      buffer_destroy(&copy);

      if (parsed.has_value) {
        number = parsed.value;
        break;
      }

      number = NAN;
      break;
    }

    case SV_BOOLEAN:
      number = var->boolean ? 1 : 0;
      break;
    case SV_NULL:
      number = 0;
      break;
    case SV_FUNCTION:
      number = NAN;
      break;
    }

  return var_create_number(number);
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

static void var_destructor(void *ptr) {
  ShellVar *var = ptr;
  var_release(var);
}

#define add_number(self, name, number)\
do { \
  ShellVar *var = var_create_number(number); \
  var_state_var_set((self), (name), var); \
  var_release(var); \
} while(0)

#define add_string(self, name, string)\
do { \
  ShellVar *var = var_create_string(string); \
  var_state_var_set((self), (name), var); \
  var_release(var); \
} while(0)

#define add_function(self, name, func_ptr, arg_count)\
do { \
  ShellVar *var = var_create_function((func_ptr), (arg_count)); \
  var_state_var_set((self), (name), var); \
  var_release(var); \
} while(0)

void var_state_init(VarState *self) {
  hash_map_init(&self->var_map, var_destructor);

  add_number(self, "PID", (double)getpid());

  add_number(self, "LAST_STATUS", 0);

  add_string(self, "LOGIN", getlogin_buffer());

  add_string(self, "HOSTNAME", gethostname_buffer());

  add_number(self, "EUID", (double)geteuid());

  add_number(self, "UID", (double)getuid());

  Buffer cwd_buf = getcwd_buffer();
  add_string(self, "PWD", buffer_clone(&cwd_buf));

  add_string(self, "OLD_PWD", cwd_buf);

  // ppwd is short for pretty print word directory
  add_string(self, "PPWD", get_pretty_cwd_buffer());

  // functions to cast
  add_function(self, "string", shell_func_string, 1);
  add_function(self, "number", shell_func_number, 1);
  add_function(self, "boolean", shell_func_boolean, 1);

  // absolute value
  add_function(self, "abs", shell_func_abs, 1);

  // some trig function
  add_function(self, "sin", shell_func_sin, 1);
  add_function(self, "cos", shell_func_cos, 1);
  add_function(self, "tan", shell_func_tan, 1);
}

void var_state_destroy(VarState *self) {
  hash_map_destroy(&self->var_map);
}

void var_state_var_set(VarState *self, const char *key, ShellVar *var) {
  hash_map_set(&self->var_map, key, var_aquire(var));
}

ShellVar *var_state_var_get(VarState *self, const char *key) {
  ShellVar *var = hash_map_get(&self->var_map, key);

  if (var == NULL) {
    return NULL;
  }

  return var_aquire(var);
}

bool var_state_var_exists(const VarState *self, const char *key) {
  return hash_map_get_const(&self->var_map, key) != NULL;
}

void var_state_var_unset(VarState *self, const char *key) {
  hash_map_remove(&self->var_map, key);
}

static void print_callback(const char *key, const void *ptr) {
  const ShellVar *var = ptr;

  const char *var_kind = var_kind_to_string(var->kind);
  Buffer var_string = var_to_string(var);

  printf("{%s}:\t%.*s (type: %s)\n", key, (int)var_string.length, var_string.char_ptr, var_kind);
  buffer_destroy(&var_string);
}

void var_state_print(const VarState *self) {
  hash_map_iter_const(&self->var_map, print_callback);
}

void var_state_update_cwd_vars(VarState *self, Buffer old_cwd) {
  ShellVar *pwd = var_create_string(getcwd_buffer());
  var_state_var_set(self, "PWD", pwd);
  var_release(pwd);

  ShellVar *old_pwd = var_create_string(old_cwd);
  var_state_var_set(self, "OLD_PWD", old_pwd);
  var_release(old_pwd);

  // ppwd is short for pretty print word directory
  ShellVar *ppwd = var_create_string(get_pretty_cwd_buffer());
  var_state_var_set(self, "PPWD", ppwd);
  var_release(ppwd);
}
