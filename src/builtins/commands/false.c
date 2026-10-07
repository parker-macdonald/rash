#include "builtins/builtin_funcs.h"

int builtin_false(char *const *argv) {
  (void)argv;

  return 1;
}
