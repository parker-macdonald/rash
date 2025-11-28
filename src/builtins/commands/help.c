#include <stdio.h>
#include <stdlib.h>

#include "../../strings/strings.h"
#include "../builtins.h"

int builtin_help(char **const argv) {
  (void)argv;

  puts(HELP_STRING);

  return EXIT_SUCCESS;
}
