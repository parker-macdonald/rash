#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../builtins.h"

#define BASE 10

static const char *const EXIT_HELP = "Usage: exit [STATUS]\n"
                              "Quit rash with the specified status code.\n"
                              "If no status code is specified, 0 is used.";

int builtin_exit(char **const argv) {
  if (argv[1] == NULL) {
    exit(0);
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(EXIT_HELP);
    return EXIT_SUCCESS;
  }

  char *endptr;
  errno = 0;
  long status = strtol(argv[1], &endptr, BASE);

  if (errno != 0 || *endptr != '\0' || status < INT_MIN || status > INT_MAX) {
    fprintf(stderr, "exit: %s: number expected\n", argv[1]);
    exit(1);
  }

  exit((int)status);

  return 0;
}
