#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtin_funcs.h"
#include "lib/error.h"
#include "rash.h"
#include "readers/interactive_reader/interactive_reader.h"

static const char *const HISTORY_HELP =
    "Usage: history [-c] [COUNT]\n"
    "View the last COUNT commands.\n"
    "If count isn't specified, all history is printed.\n"
    "When the -c option is specied, all command history is cleared and "
    "nothing\n"
    "is displayed.";

int builtin_history(char **const argv) {
  int count = -1;

  Rash *instance = rash_instance_get();

  if (!instance->interactive) {
    return EXIT_SUCCESS;
  }

  if (argv[1] != NULL) {
    if (strcmp(argv[1], "-c") == 0) {
      interactive_reader_hist_clear(&instance->interactive_reader);
      return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "--help") == 0) {
      puts(HISTORY_HELP);
      return EXIT_SUCCESS;
    }

    char *endptr;
    errno = 0;
    const long num = strtol(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || num < 0 || num > INT_MAX) {
      error_f("history: %s: positive number expected\n", argv[1]);
      return EXIT_FAILURE;
    }

    count = (int)num;
  }

  interactive_reader_hist_print(&instance->interactive_reader, count);

  return EXIT_SUCCESS;
}
