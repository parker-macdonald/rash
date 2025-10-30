#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "../builtins.h"

int builtin_export(char **const argv) {
  if (argv[1] == NULL) {
    fprintf(stderr, "Usage: %s [VAR NAME]=[VAR VALUE]\n", argv[0]);
    return EXIT_FAILURE;
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

      if (!isalnum(character) || character != '_') {
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
