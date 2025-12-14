#include <stdio.h>
#include <string.h>

#include "../builtins.h"

extern char **environ;

static const char *const ENV_HELP =
    "Usage: env\n"
    "Prints all declared environment variables.";

int builtin_env(char **argv) {
  if (argv[1] != NULL && strcmp(argv[1], "--help") == 0) {
    puts(ENV_HELP);
    return 0;
  }

  char **e = environ;
  
  while (*e != NULL) {
    printf("%s\n", *e);
    e++;
  }

  return 0;
}
