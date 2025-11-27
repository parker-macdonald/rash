#include <stdio.h>
#include <stdlib.h>

#include "../builtins.h"

const char *const HELP_STRING =
    "Welcome to rash, the rat ass shell!\n"
    "These are all of the builtin commands:\n"
    "  bg: run a paused job in the background.\n"
    "  cd: set the current working directory to the specified path.\n"
    "  exit: exit rash with an optional status code.\n"
    "  export: set an enviroment variable.\n"
    "  help: show this help text.\n"
    "  history: view and remove command history.\n"
    "  jobs: list all the paused and background jobs.\n"
    "  pwd: prints the current working directory.\n"
    "  setvar: set a shell variable.\n"
    "  true: set the status code to 0.\n"
    "  unsetvar: remove a shell variable.\n"
    "  version: display the version of rash.\n"
    "For more help with builtin commands type the command followed by --help";

int builtin_help(char **const argv) {
  (void)argv;

  puts(HELP_STRING);

  return EXIT_SUCCESS;
}
