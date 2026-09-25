#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins/builtin_funcs.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/sys.h"
#include "global.h"
#include "shell_vars/shell_vars.h"

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
      error_f("cd: HOME is not set\n");
      return EXIT_FAILURE;
    }

    path = home;
  } else if (strcmp(argv[1], "--help") == 0) {
    puts(CD_HELP);
    return EXIT_SUCCESS;
  }

  Buffer old_cwd = getcwd_buffer();

  if (chdir(path) == -1) {
    error_f("cd: %s: %s\n", path, strerror(errno));

    buffer_destroy(&old_cwd);
    return EXIT_FAILURE;
  }

  var_state_update_cwd_vars(&instance.var_state, old_cwd);

  return EXIT_SUCCESS;
}
