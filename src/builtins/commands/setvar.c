#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtins.h"
#include "lib/error.h"
#include "shell_vars.h"

static const char *const SETVAR_HELP =
    "Usage: setvar KEY VALUE\n"
    "Set the shell variable KEY equal to VALUE.";

int builtin_setvar(char **argv) {
  if (argv[1] == NULL || argv[2] == NULL) {
    error_f("%s\n", SETVAR_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(SETVAR_HELP);
    return EXIT_SUCCESS;
  }

  var_set(argv[1], argv[2]);

  return EXIT_SUCCESS;
}
