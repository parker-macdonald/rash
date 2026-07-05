#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtins.h"
#include "builtins/utils.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/parse.h"
#include "shell_vars/shell_vars.h"

static const char *const SETVAR_HELP =
    "Usage: setvar KEY [-t TYPE] VALUE\n"
    "Set the shell variable KEY equal to VALUE.";

int builtin_setvar(char **argv) {
  int argc = count_argv(argv);

  if (strcmp(argv[1], "--help") == 0) {
    puts(SETVAR_HELP);
    return EXIT_SUCCESS;
  }

  if (argc == 3) {
    char *key = argv[1];
    char *value = argv[2];

    if (strcmp(value, "null") == 0) {
      ShellVar *var = var_create_null();
      var_set(key, var);
      var_release(var);
      return EXIT_SUCCESS;
    }

    if (strcmp(value, "true") == 0) {
      ShellVar *var = var_create_boolean(true);
      var_set(key, var);
      var_release(var);
      return EXIT_SUCCESS;
    }

    if (strcmp(value, "false") == 0) {
      ShellVar *var = var_create_boolean(false);
      var_set(key, var);
      var_release(var);
      return EXIT_SUCCESS;
    }

    OptionDouble num = parse_double(value);

    if (num.has_value) {
      ShellVar *var = var_create_number(num.value);
      var_set(key, var);
      var_release(var);
      return EXIT_SUCCESS;
    }


    ShellVar *var = var_create_string(buffer_from_cstr(value));
    var_set(key, var);
    var_release(var);
    return EXIT_SUCCESS;
  }

  if (argc == 5 && strcmp(argv[2], "-t") == 0) {
    char *key = argv[1];
    char *type = argv[3];
    char *value = argv[4];

    if (strcmp(type, "string") == 0) {
      ShellVar *var = var_create_string(buffer_from_cstr(value));
      var_set(key, var);
      var_release(var);
      return EXIT_SUCCESS;
    }

    if (strcmp(type, "number") == 0) {
      OptionDouble num = parse_double(value);

      if (!num.has_value) {
        error_f("setvar: ‘%s’ is not a number.\n", value);
        return EXIT_FAILURE;
      }

      ShellVar *var = var_create_number(num.value);
      var_set(key, var);
      var_release(var);
      return EXIT_SUCCESS;
    }

    if (strcmp(type, "boolean") == 0) {
      bool boolean;
      if (strcmp(value, "true") == 0) {
        boolean = true;
      } else if (strcmp(value, "false") == 0) {
        boolean = false;
      } else {
        error_f("setvar: ‘%s’ is not a boolean.\n", value);
        return EXIT_FAILURE;
      }

      ShellVar *var = var_create_boolean(boolean);
      var_set(key, var);
      var_release(var);
      return EXIT_SUCCESS;
    }

    if (strcmp(type, "null") == 0) {
      if (strcmp(value, "null") != 0) {
        error_f("setvar: ‘%s’ is not ‘null’.\n", value);
        return EXIT_FAILURE;
      }

      ShellVar *var = var_create_null();
      var_set(key, var);
      var_release(var);
      return EXIT_SUCCESS;
    }
  }

  error_f("%s\n", SETVAR_HELP);
  return EXIT_FAILURE;
}
