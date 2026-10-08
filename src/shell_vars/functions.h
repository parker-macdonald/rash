#ifndef SHELL_VARS_FUNCTION_H
#define SHELL_VARS_FUNCTION_H

typedef struct ShellVar ShellVar;

typedef struct {
  ShellVar *(*func)(ShellVar *const *args);
  unsigned arg_count;
} ShellFunction;

// functions to cast
ShellVar *shell_func_string(ShellVar *const *args);
ShellVar *shell_func_boolean(ShellVar *const *args);
ShellVar *shell_func_number(ShellVar *const *args);

// absolute value
ShellVar *shell_func_abs(ShellVar *const *args);

// some trig function
ShellVar *shell_func_sin(ShellVar *const *args);
ShellVar *shell_func_cos(ShellVar *const *args);
ShellVar *shell_func_tan(ShellVar *const *args);

#endif
