#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../builtins.h"

static const char *const EXPORT_HELP =
    "Usage: export NAME=VALUE\n"
    "Set the enviroment variable NAME to VALUE.\n"
    "By convention, enivroment variables must start with an capital letter, \n"
    "and only contain capital letters, numbers, and underscores.";

int builtin_export(char **const argv) {
  if (argv[1] == NULL) {
    fprintf(stderr, "%s\n", EXPORT_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(EXPORT_HELP);
    return EXIT_SUCCESS;
  }

  for (size_t i = 1; argv[i] != NULL; i++) {
    if (argv[i][0] == '=' || argv[i][0] == '\0') {
      fprintf(
          stderr, "export: malformed environment variable: ‘%s’\n", argv[i]
      );
      continue;
    }

    char *separator = strchr(argv[i], '=');

    if (separator) {
      *separator = '\0';
      setenv(argv[i], separator + 1, 1);
    } else {
      setenv(argv[i], "", 1);
    }
  }

  return EXIT_SUCCESS;
}
