#ifndef SHELL_VARS_H
#define SHELL_VARS_H

#include "lib/buffer.h"
#include <stdbool.h>

typedef enum {
  SV_NUMBER,
  SV_STRING,
  SV_BOOLEAN,
  SV_NULL
} ShellVarKind;

typedef struct {
  ShellVarKind kind;
  union {
    Buffer string;
    double number;
    bool boolean;
  };
} ShellVar;

void var_init(void);

/**
 * @brief set a shell variable given a key and a value. in case it isn't
 * obvious, the data passed in is cloned.
 * @param key the key of the shell variable.
 * @param value the value of the shell variable.
 */
void var_set(const char *key, const ShellVar *var);

ShellVar *var_get(const char *key);

/**
 * @brief evaluate the shell variable from the following expression.
 * @param expr the expression to evaluate
 * @return returns the expressions value
 */
ShellVar var_eval(const char *expr);

Buffer var_to_string(const ShellVar *var);

char *var_eval_to_string(const char *expr);

/**
 * @brief unset a shell variable given a key
 * @param key the key of the variable
 * @return returns 1 if no variable was removed, returns 0 otherwise.
 */
int var_unset(const char *key);

/**
 * @brief prints all shell variables in a list.
 */
void var_print(void);

#endif
