#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int builtin_cd(char **const argv) {
  char *path = argv[1];
  if (argv[1] == NULL) {
    char *home = getenv("HOME");

    if (home == NULL) {
      fprintf(stderr, "cd: HOME is not set\n");
      return EXIT_FAILURE;
    }

    path = home;
  }

  if (chdir(path) == -1) {
    fprintf(stderr, "cd: ");
    perror(path);

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
