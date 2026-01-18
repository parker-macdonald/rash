#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "argv0.h"
#include "builtins/find_builtin.h"
#include "file_reader.h"
#include "interactive.h"
#include "interpreter/repl.h"
#include "jobs.h"
#include "lib/error.h"
#include "lib/vector.h"
#include "line_reader/line_reader.h"
#include "rashrc.h"
#include "shlvl.h"
#include "strings/version.h"

bool interactive = 0;

char *argv0;

static const char *const HELP_STRING =
    "Usage: %s [-c] [FILENAME]\n"
    "If a filename is specified, rash will run the file as a script.\n"
    "If no filename is specified rash will run in interactive mode.\n"
    "The -c option specifies one-shot mode, rash will run one command \n"
    "specified in the next argument, then exit.\n"
    "For example: \n"
    "  rash -c 'echo hello'\n"
    "rash will run 'echo hello', then exit.\n";

int main(int argc, char **argv) {
  sig_handler_init();
  trie_init();
  set_shlvl();

  // this can happen if argv is not populated in a call to execve
  if (argc == 0) {
    error_f(HELP_STRING, argv[0]);
    return 1;
  }

  argv0 = argv[0];

  // no arguments means interactive mode
  if (argc == 1) {
    interactive = true;
    load_rashrc();

    return repl(readline, NULL);
  }

  if (argc == 2) {
    if (strcmp(argv[1], "--version") == 0) {
      puts(VERSION_STRING);
      return 0;
    }

    if (strcmp(argv[1], "--help") == 0) {
      printf(HELP_STRING, argv[0]);
      return 0;
    }

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
      error_f("rash: %s: %s\n", argv[1], strerror(errno));
      return 1;
    }

    struct file_reader reader_data;
    file_reader_init(&reader_data, file);
    return repl(file_reader_read, &reader_data);
  }

  if (argc == 3) {
    // one-shot mode
    if (strcmp(argv[1], "-c") != 0) {
      error_f(HELP_STRING, argv[0]);
      return 1;
    }

    buf_t command;
    VECTOR_INIT(command);

    for (size_t i = 0; argv[2][i] != '\0'; i++) {
      if (!iscntrl((int)argv[2][i])) {
        VECTOR_PUSH(command, (uint8_t)argv[2][i]);
      }
    }

    // no need to free `command` since program exits immediately afterwards
    return repl_once(command.data);
  }

  error_f(HELP_STRING, argv[0]);
  return 1;
}
