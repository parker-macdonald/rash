#include <stdio.h>
#include <string.h>

#include "builtins/builtins.h"
#include "interpreter/repl.h"
#include "lib/vec_types.h"
#include "lib/vector.h"

extern char **environ;

static const char *const EVAL_HELP =
    "Usage: eval [arg ...]\n"
    "Join all arguments together with spaces and run them as a shell command.";

int builtin_eval(char **argv) {
  if (argv[1] == NULL || strcmp(argv[1], "--help") == 0) {
    puts(EVAL_HELP);
    return 0;
  }

  buf_t command;
  VECTOR_INIT(command);

  argv++;
  while (1) {
    buf_append_string(&command, *argv);
    argv++;
    if (*argv == NULL) {
      break;
    }
    VECTOR_PUSH(command, ' ');
  }

  int status = repl_once(command.data);

  VECTOR_DESTROY(command);

  return status;
}
