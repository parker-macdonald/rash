#include "shell_vars/shell_vars.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/hash_map.h"
#include "lib/parse.h"
#include "lib/slice.h"
#include "lib/sys.h"
#include "shell_vars/eval.h"
#include "shell_vars/lexer.h"
#include "shell_vars/token.h"

#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *const SHELL_VAR_KIND_NAMES[SV_COUNT] = {
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
  TokenList list = lex_shell_expr(&slice_lit_from_cstr(expr));

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
    default:
      unreachable();
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
    default:
      unreachable();
  }
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
    default:
      unreachable();
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

void var_state_init(VarState *self) {
  hash_map_init(&self->var_map, var_destructor);


  ShellVar *pid = var_create_number((double)getpid());
  var_state_var_set(self, "PID", pid);
  var_release(pid);

  ShellVar *last_status = var_create_number(0.0);
  var_state_var_set(self, "LAST_STATUS", last_status);
  var_release(last_status);

  ShellVar *login = var_create_string(getlogin_buffer());
  var_state_var_set(self, "LOGIN", login);
  var_release(login);

  ShellVar *hostname = var_create_string(gethostname_buffer());
  var_state_var_set(self, "HOSTNAME", hostname);
  var_release(hostname);

  ShellVar *euid = var_create_number((double)geteuid());
  var_state_var_set(self, "EUID", euid);
  var_release(euid);

  ShellVar *uid = var_create_number((double)getuid());
  var_state_var_set(self, "UID", uid);
  var_release(uid);

  ShellVar *pwd = var_create_string(getcwd_buffer());
  var_state_var_set(self, "PWD", pwd);
  var_release(pwd);

  ShellVar *old_pwd = var_create_null();
  var_state_var_set(self, "OLD_PWD", old_pwd);
  var_release(old_pwd);

  // ppwd is short for pretty print word directory
  ShellVar *ppwd = var_create_string(get_pretty_cwd_buffer());
  var_state_var_set(self, "PPWD", ppwd);
  var_release(ppwd);
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

void var_state_print(const VarState *self) {
  hash_map_iter_const(&self->var_map, print_callback);
}
