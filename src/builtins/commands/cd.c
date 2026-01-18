#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins/builtins.h"
#include "lib/f_error.h"

static const char *const CD_HELP =
    "Usage: cd [DIR]\n"
    "Set the current working directory to DIR.\n"
    "If no directory is specified, the value of the HOME enviroment variable\n"
    "is used instead.";

int builtin_cd(char **const argv) {
  const char *path = argv[1];
  if (argv[1] == NULL) {
    const char *home = getenv("HOME");

    if (home == NULL) {
      f_error("cd: HOME is not set\n");
      return EXIT_FAILURE;
    }

    path = home;
  } else if (strcmp(argv[1], "--help") == 0) {
    puts(CD_HELP);
    return EXIT_SUCCESS;
  }

  if (chdir(path) == -1) {
    f_error("cd: %s: %s\n", path, strerror(errno));

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
