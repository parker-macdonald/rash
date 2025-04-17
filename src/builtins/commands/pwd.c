#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../builtins.h"

#define MAX_CWD_SIZE 1024UL

int builtin_pwd(char **argv) {
  (void)argv;

  size_t cwd_size = 16;
  char *cwd = malloc(sizeof(char) * cwd_size);

  char *buf = getcwd(cwd, cwd_size);

  while (buf == NULL) {
    if (errno != ERANGE) {
      perror("pwd");
      free(cwd);
      return EXIT_FAILURE;
    }

    cwd_size *= 2;

    // lets not realloc forever
    if (cwd_size > PATH_MAX) {
      free(cwd);
      fprintf(stderr, "pwd: working directory exceeds %zu bytes.\n",
              MAX_CWD_SIZE);
      return EXIT_FAILURE;
    }

    cwd = realloc(cwd, cwd_size);

    buf = getcwd(cwd, cwd_size);
  }

  printf("%s\n", cwd);

  return EXIT_SUCCESS;
}
