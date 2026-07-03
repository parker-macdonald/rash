#include <stdio.h>
#include <string.h>

#include "builtins/builtins.h"
#include "interpreter/repl.h"
#include "lib/buffer.h"

extern char **environ;

static const char *const EVAL_HELP =
    "Usage: eval [arg ...]\n"
    "Join all arguments together with spaces and run them as a shell command.";

int builtin_eval(char **argv) {
  if (argv[1] == NULL || strcmp(argv[1], "--help") == 0) {
    puts(EVAL_HELP);
    return 0;
  }

  Buffer command = buffer_create(16);

  argv++;
  while (1) {
    buffer_append_cstr(&command, *argv);
    argv++;
    if (*argv == NULL) {
      break;
    }
    buffer_append_byte(&command, ' ');
  }

  int status = repl_once(&command);

  buffer_destroy(&command);

  return status;
}
