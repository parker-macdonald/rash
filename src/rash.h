#ifndef RASH_H
#define RASH_H

#include "builtins/builtins.h"
#include "jobs.h"
#include "readers/generic_reader.h"
#include "readers/interactive_reader/types.h"
#include "shell_vars/shell_vars.h"

typedef struct {
  char *argv0;
  bool interactive;
  // file desciptor of the open tty or -1 if the session is not interactive
  int tty_fd;

  // these two are only filled in if interactive is true
  Buffer interactive_prompt;
  InteractiveReader interactive_reader;

  GenericReader reader;

  Builtins builtins;
  Jobs jobs;
  VarState var_state;
} Rash;

typedef enum {
  // expected failures, like the user types `rash --help`, no instance will be created in that case
  EXPECTED_FAILURE,
  // unexpected failures, some subsystem failed to initialize or invalid parameters were provided
  UNEXPECTED_FAILURE,
  // everything went ok!
  INIT_SUCCESS
} RashInstanceInitResult;

RashInstanceInitResult rash_instance_init(Rash *out, int argc, char **argv);

void rash_instance_delete(Rash *self);

#endif
