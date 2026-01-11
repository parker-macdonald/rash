#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins/builtins.h"
#include "builtins/find_builtin.h"
#include "lib/search_path.h"

static const char *const WHICH_HELP =
    "Usage: which FILE...\n"
    "Checks if FILE can be executed. If FILE can be executed, it is either a \n"
    "shell builtin, a path to an executable, or an executable in the users \n"
    "PATH enviroment variable.";

int builtin_which(char **argv) {
  if (argv[1] == NULL) {
    (void)fprintf(stderr, "%s\n", WHICH_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(WHICH_HELP);
    return EXIT_SUCCESS;
  }

  for (size_t i = 1; argv[i] != NULL; i++) {
    if (strchr(argv[i], '/') != NULL) {
      if (faccessat(AT_FDCWD, argv[i], X_OK, AT_EACCESS) == 0) {
        printf("executable: %s\n", argv[i]);
        continue;
      }
    }

    if (find_builtin(argv[i]) != NULL) {
      printf("%s: shell builtin\n", argv[i]);
      continue;
    }

    char *file = search_path(argv[i]);

    if (file != NULL) {
      printf("executable: %s\n", file);
      free(file);
    } else {
      printf("cannot be executed: %s\n", argv[i]);
    }
  }

  return EXIT_SUCCESS;
}
