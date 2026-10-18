#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtin_funcs.h"
#include "lib/error.h"
#include "lib/parse.h"

static const char *const EXIT_HELP =
    "Usage: exit [STATUS]\n"
    "Quit rash with the specified status code.\n"
    "If no status code is specified, 0 is used.";

int builtin_exit(char *const *argv) {
  if (argv[1] == NULL) {
    exit(0);
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(EXIT_HELP);
    return EXIT_SUCCESS;
  }

  OptionInt num = parse_int(argv[1]);

  if (!num.has_value) {
    error_f("exit: %s: number expected\n", argv[1]);
    exit(1);
  }

  exit(num.value);

  return 0;
}
