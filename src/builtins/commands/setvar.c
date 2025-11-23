#include "../../shell_vars.h"
#include <stdio.h>
#include <stdlib.h>

int builtin_setvar(char **argv) {
  if (argv[1] == NULL || argv[2] == NULL) {
    fputs("Usage: setvar KEY VALUE\n",stderr);
    return EXIT_FAILURE;
  }

  set_var(argv[1], argv[2]);

  return EXIT_SUCCESS;
}
