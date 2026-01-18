#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtins.h"
#include "lib/error.h"
#include "shell_vars.h"

static const char *const UNSETVAR_HELP = "Usage: unsetvar KEY\n"
                                         "Remove the shell variable KEY.";

int builtin_unsetvar(char **argv) {
  if (argv[1] == NULL) {
    error_f("%s\n", UNSETVAR_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(UNSETVAR_HELP);
    return EXIT_SUCCESS;
  }

  if (var_unset(argv[1]) == 1) {
    error_f("unsetvar: shell variable ‘%s’ was not declared.\n", argv[1]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
