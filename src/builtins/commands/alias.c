#include "alias.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtins.h"
#include "interpreter/lex.h"
#include "lib/error.h"

static const char *const ALIAS_HELP =
    "Usage: alias [-p] [NAME=VALUE...]\n"
    "Alias NAME to VALUE. So when typing a command with NAME as the first\n"
    "argument VALUE is used instead.\n"
    "If no arguments are provided or the -p option is specified, all\n"
    "registered aliases are printed";

int builtin_alias(char **const argv) {
  if (argv[1] == NULL) {
    alias_print();
    return EXIT_SUCCESS;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(ALIAS_HELP);
    return EXIT_SUCCESS;
  }

  if (strcmp(argv[1], "-p") == 0) {
    alias_print();
    return EXIT_SUCCESS;
  }

  for (size_t i = 1; argv[i] != NULL; i++) {
    if (argv[i][0] == '=' || argv[i][0] == '\0') {
      error_f("export: malformed alias: ‘%s’\n", argv[i]);
      continue;
    }

    char *separator = strchr(argv[i], '=');

    if (separator) {
      *separator = '\0';
      token_t *tokens = lex((uint8_t *)separator);

      if (tokens == NULL) {
        return EXIT_FAILURE;
      }

      alias_set(argv[i], tokens);
    } else {
      error_f("alias: expected ‘=’ after alias name (%s).\n", argv[i]);
    }
  }

  return EXIT_SUCCESS;
}
