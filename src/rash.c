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

RashInstanceInitResult rash_instance_from_file(Rash *out, const char *filename, char *argv0) {
  if (file_reader_create(&out->reader, filename)) {
    error_f("rash: %s: %s\n", filename, strerror(errno));
    return UNEXPECTED_FAILURE;
  }

  out->argv0 = argv0;
  out->tty_fd = -1;
  out->interactive = false;

  builtins_init(&out->builtins);
  var_state_init(&out->var_state);
  jobs_init(&out->jobs, -1);

  set_shlvl();

  return INIT_SUCCESS;
}

RashInstanceInitResult rash_instance_interactive(Rash *out, char *argv0) {
  int tty_fd = open("/dev/tty", O_RDWR, 0666);

  if (tty_fd == -1) {
    error_f("rash: cannot access /dev/tty (%s). Assuming this session is non-interactive.\n", strerror(errno));
    return rash_instance_from_file(out, "/dev/stdin", argv0);
  }

  if (!isatty(tty_fd)) {
    (void)close(tty_fd);

    error("rash: /dev/tty is not a terminal. Assuming this session is non-interactive.\n");

    return rash_instance_from_file(out, "/dev/stdin", argv0);
  }

  out->argv0 = argv0;

  interactive_reader_init(&out->interactive_reader);
  generic_reader_from_interactive(&out->reader, &out->interactive_reader);
  out->interactive = true;
  out->tty_fd = tty_fd;
  out->interactive_prompt = buffer_from_cstr("LOGIN + \"@\" + HOSTNAME + \" \" + PWD + \" \" + (LAST_STATUS == 0 ? \":)\" : \":(\") + \" $ \"");
  
  builtins_init(&out->builtins);
  var_state_init(&out->var_state);
  jobs_init(&out->jobs, tty_fd);

  set_shlvl();

  load_rashrc();

  return INIT_SUCCESS;
}

RashInstanceInitResult rash_instance_one_shot(Rash *out, int argc, char **argv) {
  Buffer command = buffer_create(16);

  for (size_t i = 2; i < (size_t)argc; i++) {
    for (size_t j = 0; argv[i][j] != '\0'; j++) {
      if (!iscntrl((int)argv[i][j])) {
        buffer_append(&command, argv[i][j]);
      }
    }

    buffer_append(&command, ' ');
  }

  one_shot_reader_create(&out->reader, command);

  out->argv0 = argv[0];
  out->tty_fd = -1;
  out->interactive = false;

  builtins_init(&out->builtins);
  var_state_init(&out->var_state);
  jobs_init(&out->jobs, -1);

  set_shlvl();

  return INIT_SUCCESS;

}

RashInstanceInitResult rash_instance_init(Rash *out, int argc, char **argv) {
  // this can happen (on some systems but not linux) if argv is not populated in a call to execve
  if (argc == 0) {
    error_f(HELP_STRING, argv[0]);
    return UNEXPECTED_FAILURE;
  }

  if (argc == 2 && strcmp(argv[1], "--version") == 0) {
    puts(VERSION_STRING);
    return EXPECTED_FAILURE;
  }

  if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    printf(HELP_STRING, argv[0]);
    return EXPECTED_FAILURE;
  }

  // no arguments means interactive mode
  if (argc == 1) {
    return rash_instance_interactive(out, argv[0]);
  }

  if (argc == 2) {
    return rash_instance_from_file(out, argv[1], argv[0]);
  }

  if (argc >= 3) {
    // one-shot mode
    if (strcmp(argv[1], "-c") != 0) {
      error_f(HELP_STRING, argv[0]);
      return UNEXPECTED_FAILURE;
    }

    return rash_instance_one_shot(out, argc, argv);
  }

  error_f(HELP_STRING, argv[0]);
  return UNEXPECTED_FAILURE;
}

void rash_instance_delete(Rash *rash) {
  builtins_destroy(&rash->builtins);
  jobs_destroy(&rash->jobs);
  var_state_destroy(&rash->var_state);
  generic_reader_destroy(&rash->reader);
}
