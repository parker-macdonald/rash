#include <stdio.h>
#include <stdlib.h>

#include "builtins/builtins.h"

static const char *const HELP_STRING =
    "Welcome to rash, the rat ass shell!\n"
    "These are all of the builtin commands:\n"
    "  bg: run a paused job in the background.\n"
    "  cd: set the current working directory to the specified path.\n"
    "  env: list all environment variables.\n"
    "  exec: replace rash with a specified program.\n"
    "  exit: exit rash with an optional status code.\n"
    "  export: set an enviroment variable.\n"
    "  fg: run a paused job in the foreground.\n"
    "  help: show this help text.\n"
    "  history: view and remove command history.\n"
    "  jobs: list all the paused and background jobs.\n"
    "  pwd: prints the current working directory.\n"
    "  setvar: set an environment variable.\n"
    "  setvar: set a shell variable.\n"
    "  source: read and execute the commands from a specified file.\n"
    "  true: set the status code to 0.\n"
    "  unsetenv: remove an environment variable.\n"
    "  unsetvar: remove a shell variable.\n"
    "  var: list all shell variables.\n"
    "  version: display the version of rash.\n"
    "  which: search the PATH environment variable for an executable.\n"
    "For more help with builtin commands type the command followed by --help";

int builtin_help(char **const argv) {
  (void)argv;

  puts(HELP_STRING);

  return EXIT_SUCCESS;
}
