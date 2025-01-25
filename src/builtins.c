#include "builtins.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern bool should_exit;

int builtin_cd(char **const argv) {
  char *path = argv[1];
  if (argv[1] == NULL) {
    char *home = getenv("HOME");

    if (home == NULL) {
      fprintf(stderr, "HOME is not set\n");
      return EXIT_FAILURE;
    }

    path = home;
  }

  if (chdir(path) == -1) {
    fprintf(stderr, "%s: ", argv[0]);
    perror(path);

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int builtin_help(char **const argv) {
  printf("Using rash from path: %s\n", argv[0]);

  return EXIT_SUCCESS;
}

int builtin_exit(char **const argv) {
  should_exit = true;

  if (argv[1] == NULL) {
    return 0;
  }

  int old_errno = errno;
  long status = strtol(argv[1], NULL, 10);

  if (old_errno != errno) {
    perror("exit");
    return 2;
  }

  return (int)status;
}

int builtin_export(char **const argv) {
  if (argv[1] == NULL) {
    fprintf(stderr, "Usage: %s [VAR NAME]=[VAR VALUE]", argv[1]);
    return EXIT_FAILURE;
  }

  for (size_t i = 1; argv[i] != NULL; i++) {
    if (!isalpha(argv[i][0])) {
      fprintf(stderr, "export: Invalid identifier: `%s`\n", argv[i]);
      continue;
    }
    
    char* env_value = NULL;

    for (size_t j = 1; argv[i][j] != '\0'; j++) {
      char c = argv[i][j];
      if (c == '=') {
        argv[i][j] = '\0';
        env_value = &argv[i][j] + 1;
        break;
      }

      if (!isalnum(c)) {
        fprintf(stderr, "export: Invalid identifier: `%s`\n", argv[i]);
        continue;
      }
    }

    if (setenv(argv[i], env_value, 1) != 0) {
      fprintf(stderr, "export: ");
      perror(argv[i]);
    }
  }

  return EXIT_SUCCESS;
}

const char *const builtins[NUM_OF_BUILTINS] = {"cd", "help", "exit", "export"};

int (*builtin_fns[NUM_OF_BUILTINS])(char **const) = {
    &builtin_cd, &builtin_help, &builtin_exit, &builtin_export};
