#include <stdio.h>
#include <string.h>

#include "../../shell_vars.h"
#include "builtins/builtins.h"

static const char *const VAR_HELP = "Usage: var\n"
                                    "Prints all declared shell variables.";

int builtin_var(char **argv) {
  if (argv[1] != NULL && strcmp(argv[1], "--help") == 0) {
    puts(VAR_HELP);
    return 0;
  }

  var_print();

  return 0;
}
