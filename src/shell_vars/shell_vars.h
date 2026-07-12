#ifndef SHELL_VARS_H
#define SHELL_VARS_H

#include "lib/buffer.h"
#include <stdbool.h>

typedef enum {
  SV_NUMBER,
  SV_STRING,
  SV_BOOLEAN,
  SV_NULL,
  SV_COUNT
} ShellVarKind;

extern const char *SHELL_VAR_KIND_NAMES[SV_COUNT];

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

ShellVar *var_cast_to_string(const ShellVar *var);
ShellVar *var_cast_to_boolean(const ShellVar *var);
ShellVar *var_cast_to_number(const ShellVar *var);

ShellVar *var_eval(const char *expr);

char *var_eval_to_string(const char *expr);

// functions below for messing with the internal hashmap of shellvars to
// identifiers

void var_init(void);

/**
 * @brief prints all registered shell variables in a list.
 */
void var_print(void);

void var_set(const char *key, ShellVar *var);

ShellVar *var_get(const char *key);

/**
 * @brief unset a shell variable given a key
 * @param key the key of the variable
 */
void var_unset(const char *key);

#endif
