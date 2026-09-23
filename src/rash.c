#include "rash.h"
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "builtins/builtins.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "rashrc.h"
#include "readers/file_reader.h"
#include "readers/generic_reader.h"
#include "readers/interactive_reader/interactive_reader.h"
#include "readers/one_shot_reader.h"
#include "shell_vars/shell_vars.h"
#include "jobs.h"
#include "shlvl.h"
#include "strings/version.h"

static const char *const HELP_STRING =
    "Usage: %s [-c] [FILENAME]\n"
    "If a filename is specified, rash will run the file as a script.\n"
    "If no filename is specified rash will run in interactive mode.\n"
    "The -c option specifies one-shot mode, rash will run one command \n"
    "specified in the next argument, then exit.\n"
    "For example: \n"
    "  rash -c 'echo hello'\n"
    "rash will run 'echo hello', then exit.\n";

Rash rash_instance_init(int argc, char **argv) {
  // this can happen (on some systems but not linux) if argv is not populated in a call to execve
  if (argc == 0) {
    error_f(HELP_STRING, argv[0]);
    exit(1);
  }

  if (argc == 2 && strcmp(argv[1], "--version") == 0) {
    puts(VERSION_STRING);
    exit(0);
  }

  if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    printf(HELP_STRING, argv[0]);
    exit(0);
  }

  Rash rash;
  rash.argv0 = argv[0];
  rash.interactive = false;
  rash.tty_fd = -1;
  builtins_init(&rash.builtins);
  var_state_init(&rash.var_state);

  set_shlvl();

  // no arguments means interactive mode
  if (argc == 1) {
    int tty_fd = open("/dev/tty", O_RDWR, 0666);

    if (tty_fd == -1) {
      error_f("rash: cannot access /dev/tty (%s). Job control is unavailable.\n", strerror(errno));

      file_reader_create(&rash.reader, "/dev/stdin");
      jobs_init(&rash.jobs, -1);

      return rash;
    }

    interactive_reader_init(&rash.interactive_reader);
    generic_reader_from_interactive(&rash.reader, &rash.interactive_reader);
    jobs_init(&rash.jobs, tty_fd);
    rash.interactive = true;
    rash.tty_fd = tty_fd;
    rash.interactive_prompt = buffer_from_cstr("LOGIN + \"@\" + HOSTNAME + \" \" + PWD + \" \" + (LAST_STATUS == 0 ? \":)\" : \":(\") + \" $ \"");

    return rash;
  }

  jobs_init(&rash.jobs, -1);

  if (argc == 2) {
    if (file_reader_create(&rash.reader, argv[1])) {
      error_f("rash: %s: %s\n", argv[1], strerror(errno));
      exit(1);
    }

    return rash;
  }

  if (argc == 3) {
    // one-shot mode
    if (strcmp(argv[1], "-c") != 0) {
      error_f(HELP_STRING, argv[0]);
      exit(1);
    }

    Buffer command = buffer_create(16);

    for (size_t i = 2; i < (size_t)argc; i++) {
      for (size_t j = 0; argv[i][j] != '\0'; i++) {
        if (!iscntrl((int)argv[i][j])) {
          buffer_append(&command, argv[i][j]);
        }
      }

      buffer_append(&command, ' ');
    }

    one_shot_reader_create(&rash.reader, command);

    return rash;
  }

  error_f(HELP_STRING, argv[0]);
  exit(1);
}

void rash_instance_delete(Rash *rash) {
  builtins_destroy(&rash->builtins);
  jobs_destroy(&rash->jobs);
  var_state_destroy(&rash->var_state);
  generic_reader_destroy(&rash->reader);
}

static Rash *instance;

Rash *rash_instance_get(void) {
  return instance;
}

void rash_register_global_instance(Rash *rash) {
  instance = rash;
}