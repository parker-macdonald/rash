#ifndef SHELL_VARS_H
#define SHELL_VARS_H

#include "lib/buffer.h"
#include "lib/optional.h"
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
  union {
    Buffer string;
    double number;
    bool boolean;
  };
} ShellVar;

typedef OPTIONAL(ShellVar) OptionShellVar;

void var_destroy(ShellVar *var);

ShellVar var_clone(const ShellVar *var);

Buffer var_to_string(const ShellVar *var);

void var_init(void);

void var_set(const char *key, const ShellVar *var);

/**
 * @brief unset a shell variable given a key
 * @param key the key of the variable
 * @return returns 1 if no variable was removed, returns 0 otherwise.
 */
int var_unset(const char *key);

ShellVar *var_get(const char *key);

OptionShellVar var_eval(const char *expr);

char *var_eval_to_string(const char *expr);

/**
 * @brief prints all shell variables in a list.
 */
void var_print(void);

#endif
