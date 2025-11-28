#include <ctype.h>
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
    if (!isalpha(argv[i][0])) {
      fprintf(stderr, "export: Invalid identifier: ‘%s’\n", argv[i]);
      continue;
    }

    char *env_value = NULL;

    for (size_t j = 1; argv[i][j] != '\0'; j++) {
      char character = argv[i][j];
      if (character == '=') {
        argv[i][j] = '\0';
        env_value = &argv[i][j] + 1;
        break;
      }

      if (!isalnum(character) && character != '_') {
        fprintf(stderr, "export: Invalid identifier: ‘%s’\n", argv[i]);
        continue;
      }
    }

    if (setenv(argv[i], env_value ? env_value : "", 1) != 0) {
      fprintf(stderr, "export: ");
      perror(argv[i]);
    }
  }

  return EXIT_SUCCESS;
}
