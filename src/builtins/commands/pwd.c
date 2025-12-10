#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../builtins.h"

static const char *const PWD_HELP = "Usage: pwd\n"
                                    "Prints the current working directory.";

int builtin_pwd(char **argv) {
  if (argv[1] != NULL && strcmp(argv[1], "--help") == 0) {
    puts(PWD_HELP);
    return EXIT_SUCCESS;
  }

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
      fprintf(
          stderr, "pwd: out of memory, current working directory too long.\n"
      );
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
