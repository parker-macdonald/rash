#include "builtins.h"
#include "enviroment.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int builtin_cd(char **const argv) {
  if (argv[1] == NULL) {
    fprintf(stderr, "Usage: %s [PATH]\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (chdir(argv[1]) == -1) {
    fprintf(stderr, "%s: ", argv[0]);
    perror(argv[1]);

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int builtin_help(char **const argv) {
  printf("Example help command\n");

  return EXIT_SUCCESS;
}

int builtin_exit(char **const argv) { exit(EXIT_SUCCESS); }

int builtin_export(char **const argv) {
  if (argv[1] == NULL) {
    fprintf(stderr, "Usage: %s [VAR NAME]=[VAR VALUE]", argv[1]);
    return EXIT_FAILURE;
  }

  for (size_t i = 1; argv[i] != NULL; i++) {
    for (size_t j = 0; argv[i][j] != '\0'; j++) {
      char c = argv[i][j];
      if (c == '=') {
        break;
      }

      if (!isalpha(c)) {
        fprintf(stderr, "Invalid identifier: `%s`\n", argv[i]);
        return EXIT_FAILURE;
      }
    }

    env_add(argv[i]);
  }

  return EXIT_SUCCESS;
}

const char *const builtins[NUM_OF_BUILTINS] = {"cd", "help", "exit", "export"};

int (*builtin_fns[NUM_OF_BUILTINS])(char **const) = {
    &builtin_cd, &builtin_help, &builtin_exit, &builtin_export};