#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../line_reader/line_reader.h"
#include "../builtins.h"

#define BASE 10

int builtin_history(char **const argv) {
  int count = -1;

  if (argv[1] != NULL) {
    if (strcmp(argv[1], "-c") == 0) {
      clear_history();
      return EXIT_SUCCESS;
    }

    char *endptr;
    errno = 0;
    const long num = strtol(argv[1], &endptr, BASE);
    if (errno != 0 || *endptr != '\0' || num < 0 || num > INT_MAX) {
      fprintf(stderr, "history: %s: positive number expected\n", argv[1]);
      return EXIT_FAILURE;
    }

    count = (int)num;
  }

  print_history(count);

  return EXIT_SUCCESS;
}
