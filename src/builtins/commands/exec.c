#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins/builtins.h"
#include "builtins/find_builtin.h"
#include "interactive.h"
#include "lib/search_path.h"

extern char **environ;

static const char *const EXEC_HELP =
    "Usage: exec COMMAND [ARGS...]\n"
    "Replace rash with the command given. If the command fails to run, rash\n"
    "continues to run in interactive mode and exits in scripting mode.";

int builtin_exec(char **const argv) {
  if (argv[1] == NULL) {
    puts(EXEC_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(EXEC_HELP);
    return EXIT_SUCCESS;
  }

  builtin_t builtin = find_builtin(argv[1]);

  if (builtin != NULL) {
    _exit(builtin(&argv[1]));
  }

  // search path for executable
  if (strchr(argv[1], '/') == NULL) {
    char *argv0 = search_path(argv[1]);

    if (argv0 == NULL) {
      (void)fprintf(stderr, "%s: command not found\n", argv[1]);

      if (interactive) {
        return EXIT_FAILURE;
      }
      _exit(EXIT_FAILURE);
    }

    free(argv[1]);
    argv[1] = argv0;
  }

  int status = execve(argv[1], &argv[1], environ);

  if (status == -1) {
    perror("rash");
  }

  if (interactive) {
    return EXIT_FAILURE;
  }

  _exit(EXIT_FAILURE);
}
