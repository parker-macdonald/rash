#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../builtins.h"

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

    char *temp_cwd = realloc(cwd, cwd_size);

    if (temp_cwd == NULL) {
      fprintf(stderr,
              "pwd: out of memory, current working directory too long.\n");
      free(cwd);
      return EXIT_FAILURE;
    } else {
      cwd = temp_cwd;
    }

    buf = getcwd(cwd, cwd_size);
  }

  printf("%s\n", cwd);
  free(cwd);

  return EXIT_SUCCESS;
}
