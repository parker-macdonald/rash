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
#include "shell_vars/functions.h"
#include "shell_vars/shell_vars.h"

static const char *const CD_HELP =
    "Usage: cd [DIR]\n"
    "Set the current working directory to DIR.\n"
    "If no directory is specified, the value of the HOME enviroment variable\n"
    "is used instead.";

int builtin_cd(char *const *argv) {
  if (argv[1] && strcmp(argv[1], "--help") == 0) {
    puts(CD_HELP);
    return EXIT_SUCCESS;
  }
  
  Buffer path;

  if (argv[1] != NULL) {
    if (strcmp(argv[1], "-") == 0) {
      ShellVar *old_pwd = var_state_var_get(&instance.var_state, "OLD_PWD");

      if (old_pwd == NULL || old_pwd->kind != SV_STRING) {
        error("cd: var OLD_PWD is not set or is not a string, cannot cd into previous directory.\n");
        return EXIT_FAILURE;
      }

      var_release(old_pwd);
      path = buffer_clone(&old_pwd->string);
    } else {
      path = buffer_from_cstr(argv[1]);
    }
  } else {
    if (getenv_buffer("HOME", &path)) {
      error_f("cd: HOME is not set\n");
      return EXIT_FAILURE;
    }
  }

  Buffer old_cwd = getcwd_buffer();

  if (chdir(buffer_cstr(&path)) == -1) {
    error_f("cd: %.*s: %s\n", (int)path.length, path.char_ptr, strerror(errno));

    buffer_destroy(&path);
    buffer_destroy(&old_cwd);
    return EXIT_FAILURE;
  }

  buffer_destroy(&path);
  var_state_update_cwd_vars(&instance.var_state, old_cwd);

  return EXIT_SUCCESS;
}
