#include "shell_vars/functions.h"
#include "shell_vars/shell_vars.h"

#include <math.h>

// functions to cast
ShellVar *shell_func_string(ShellVar *const *args) {
  return var_cast_to_string(args[0]);
}
ShellVar *shell_func_boolean(ShellVar *const *args) {
  return var_cast_to_boolean(args[0]);
}
ShellVar *shell_func_number(ShellVar *const *args) {
  return var_cast_to_number(args[0]);
}

// absolute value
ShellVar *shell_func_abs(ShellVar *const *args) {
  if (args[0]->kind != SV_NUMBER) {
    return var_create_number(NAN);
  }

  return var_create_number(fabs(args[0]->number));
}

// some trig function
ShellVar *shell_func_sin(ShellVar *const *args) {
  if (args[0]->kind != SV_NUMBER) {
    return var_create_number(NAN);
  }

  return var_create_number(sin(args[0]->number));
}
ShellVar *shell_func_cos(ShellVar *const *args) {
  if (args[0]->kind != SV_NUMBER) {
    return var_create_number(NAN);
  }

  return var_create_number(cos(args[0]->number));
}
ShellVar *shell_func_tan(ShellVar *const *args) {
  if (args[0]->kind != SV_NUMBER) {
    return var_create_number(NAN);
  }

  return var_create_number(tan(args[0]->number));
}
