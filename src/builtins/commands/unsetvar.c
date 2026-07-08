#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtins.h"
#include "lib/error.h"
#include "shell_vars/shell_vars.h"
#include "shell_vars/util.h"

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

  if (!is_ident_cstr(argv[1])) {
    error_f("setvar: `%s` is not a valid identifier.\n", argv[1]);
    return EXIT_FAILURE;
  }

  var_unset(argv[1]);

  return EXIT_SUCCESS;
}
