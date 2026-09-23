#ifndef SHELL_VARS_H
#define SHELL_VARS_H

#include "lib/buffer.h"
#include "lib/hash_map.h"
#include <stdbool.h>

typedef enum {
  SV_NUMBER,
  SV_STRING,
  SV_BOOLEAN,
  SV_NULL,
  SV_COUNT
} ShellVarKind;

extern const char *const SHELL_VAR_KIND_NAMES[SV_COUNT];

typedef struct {
  ShellVarKind kind;
  unsigned ref_count;
  union {
    Buffer string;
    double number;
    bool boolean;
  };
} ShellVar;

ShellVar *var_create_string(Buffer string);
ShellVar *var_create_number(double number);
ShellVar *var_create_boolean(bool boolean);
ShellVar *var_create_null(void);

ShellVar *var_aquire(ShellVar *var);
void var_release(ShellVar *var);

Buffer var_to_string(const ShellVar *var);
bool var_to_boolean(const ShellVar *var);

ShellVar *var_cast_to_string(const ShellVar *var);
ShellVar *var_cast_to_boolean(const ShellVar *var);
ShellVar *var_cast_to_number(const ShellVar *var);

ShellVar *var_eval(const char *expr);

char *var_eval_to_string(const char *expr);

// functions below for messing with the internal hashmap of shellvars to
// identifiers

typedef struct {
  HashMap var_map;
} VarState;

void var_state_init(VarState *self);

void var_state_destroy(VarState *self);

/**
 * @brief prints all registered shell variables in a list.
 */
void var_state_print(const VarState *self);

void var_state_var_set(VarState *self, const char *key, ShellVar *var);

ShellVar *var_state_var_get(VarState *self, const char *key);

bool var_state_var_exists(const VarState *self, const char *key);

/**
 * @brief unset a shell variable given a key
 * @param key the key of the variable
 */
void var_state_var_unset(VarState *self, const char *key);

#endif
