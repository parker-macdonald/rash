#include "builtins/builtins.h"
#include "builtins/utils.h"
#include <stdio.h>

int builtin_true(char **argv) {
  printf("%d\n", count_argv(argv));

  return 0;
}
