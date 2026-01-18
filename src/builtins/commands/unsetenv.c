#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtins.h"
#include "lib/f_error.h"

static const char *const UNSETENV_HELP = "Usage: unsetenv KEY\n"
                                         "Remove the environment variable KEY.";

int builtin_unsetenv(char **argv) {
  if (argv[1] == NULL) {
    f_error("%s\n", UNSETENV_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(UNSETENV_HELP);
    return EXIT_SUCCESS;
  }

  if (argv[1][0] == '\0' || strchr(argv[1], '=') != NULL) {
    f_error("unsetenv: malformed key: ‘%s’\n", argv[1]);
    return EXIT_FAILURE;
  }

  if (getenv(argv[1]) == NULL) {
    f_error("unsetenv: environment variable ‘%s’ was not set.\n", argv[1]);
    return EXIT_FAILURE;
  }

  unsetenv(argv[1]);

  return EXIT_SUCCESS;
}
