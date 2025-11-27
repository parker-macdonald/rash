#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../shell_vars.h"
#include "../builtins.h"

const char *const UNSETVAR_HELP = "Usage: unsetvar KEY\n"
                                  "Remove the shell variable KEY.";

int builtin_unsetvar(char **argv) {
  if (argv[1] == NULL) {
    fprintf(stderr, "%s\n", UNSETVAR_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(UNSETVAR_HELP);
    return EXIT_SUCCESS;
  }

  if (unset_var(argv[1]) == 1) {
    fprintf(
        stderr, "unsetvar: shell variable ‘%s’ was not declared.\n", argv[1]
    );
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
