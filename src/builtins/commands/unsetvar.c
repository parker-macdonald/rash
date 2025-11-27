#include <stdio.h>
#include <stdlib.h>

#include "../../shell_vars.h"
#include "../builtins.h"

int builtin_unsetvar(char **argv) {
  if (argv[1] == NULL) {
    fputs("Usage: unsetvar KEY\n", stderr);
    return EXIT_FAILURE;
  }

  if (unset_var(argv[1]) == 1) {
    fprintf(
        stderr, "unsetvar: shell variable ‘%s’ was not declared.\n", argv[1]
    );
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
