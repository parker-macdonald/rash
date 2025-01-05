#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int builtin_cd(char **const argv) {
  if (argv[1] == NULL) {
    fprintf(stderr, "Usage: %s [PATH]\n", argv[1]);
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

const char *const builtins[NUM_OF_BUILTINS] = {"cd", "help", "exit"};

int (*builtin_fns[NUM_OF_BUILTINS])(char **const) = {&builtin_cd, &builtin_help,
                                                     &builtin_exit};