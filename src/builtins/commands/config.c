#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins/builtin_funcs.h"
#include "lib/buffer.h"
#include "global.h"

static const char *const CONFIG_HELP =
"Usage: config get|set KEY VALUE\n"
"Set the config key to a value. Currently the only config key is `interactive.prompt`\n"
"which can be set to a shell expression (which is a string).\n"
"For example:\n"
"`config set interactive.prompt 'LOGIN + \"@\" + HOST + \":\" + PPWD + (EUID == 0 ? \"#\" : \"$\") + \" \"`";

int builtin_config(char *const *argv) {
  if (argv[1] == NULL) {
    puts(CONFIG_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(CONFIG_HELP);
    return EXIT_SUCCESS;
  }

  if (strcmp(argv[1], "get") == 0) {
    if (argv[2] == NULL) {
      (void)fprintf(stderr, "config: expected config key after `set`.\n");
      return EXIT_FAILURE;
    }

    if (strcmp(argv[2], "interactive.prompt") == 0) {
      if (!instance.interactive) {
        (void)fprintf(stderr, "config: this rash instance is not interactive, cannot get `interactive.prompt`.\n");
        return EXIT_FAILURE;
      }

      printf("%.*s\n", (int)instance.interactive_prompt.length, instance.interactive_prompt.char_ptr);
      return EXIT_SUCCESS;
    }

    (void)fprintf(stderr, "config: no such config key `%s`.\n", argv[2]);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "set") == 0) {
    if (argv[2] == NULL) {
      (void)fprintf(stderr, "config: expected config key after `set`.\n");
      return EXIT_FAILURE;
    }

    if (argv[3] == NULL) {
      (void)fprintf(stderr, "config: expected value after config key `%s`.\n", argv[2]);
      return EXIT_FAILURE;
    }

    if (strcmp(argv[2], "interactive.prompt") == 0) {
      if (!instance.interactive) {
        (void)fprintf(stderr, "config: this rash instance is not interactive, cannot set `interactive.prompt`.\n");
        return EXIT_FAILURE;
      }

      Buffer new_interactive_prompt = buffer_from_cstr(argv[3]);
      buffer_destroy(&instance.interactive_prompt);
      instance.interactive_prompt = new_interactive_prompt;
      return EXIT_SUCCESS;
    }

    (void)fprintf(stderr, "config: no such config key `%s`.\n", argv[2]);
    return EXIT_FAILURE;
  }

  (void)fprintf(stderr, "config: unknown subcommand `%s`, try `get`, `set`, or `--help`.\n", argv[1]);
  return EXIT_SUCCESS;
}
