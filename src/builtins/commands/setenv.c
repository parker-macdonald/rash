#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtins.h"

static const char *const SETENV_HELP =
    "Usage: setenv KEY [VALUE]\n"
    "Set the environment variable KEY equal to VALUE.\n"
    "If value isn't specified, the shell variable has no value.";

int builtin_setenv(char **argv) {
  if (argv[1] == NULL) {
    (void)fprintf(stderr, "%s\n", SETENV_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(SETENV_HELP);
    return EXIT_SUCCESS;
  }

  if (argv[1][0] == '\0' || strchr(argv[1], '=') != NULL) {
    (void)fprintf(stderr, "setenv: malformed key: ‘%s’\n", argv[1]);
    return EXIT_FAILURE;
  }

  setenv(argv[1], argv[2] ? argv[2] : "", 1);

  return EXIT_SUCCESS;
}
