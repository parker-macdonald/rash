#include "utils.h"
#include <stddef.h>

int count_argv(char *const *argv) {
  int count = 0;

  while (*argv != NULL) {
    count++;
    argv++;
  }

  return count;
}
