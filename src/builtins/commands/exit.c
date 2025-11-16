#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../should_exit.h"
#include "../builtins.h"

#define BASE 10

int builtin_exit(char **const argv) {
  should_exit = true;

  if (argv[1] == NULL) {
    return 0;
  }

  char *endptr;
  errno = 0;
  long status = strtol(argv[1], &endptr, BASE);

  if (errno != 0 || *endptr != '\0' || status < INT_MIN || status > INT_MAX) {
    fprintf(stderr, "exit: %s: number expected\n", argv[1]);
    return EXIT_FAILURE;
  }

  return (int)status;
}
