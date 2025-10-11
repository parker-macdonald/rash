#ifndef EXECUTE_H
#define EXECUTE_H

#include "optional.h"

typedef struct {
  // arguments for command
  char **argv;
  // fd to redirect stdout, or -1
  int stdout_fd;
  // fd to redirect stdin, or -1
  int stdin_fd;
  // fd to redirect stderr, or -1
  int stderr_fd;
  int flags;
} execution_context;

typedef OPTIONAL(execution_context) optional_exec_context;

int execute(const execution_context context);

#endif
