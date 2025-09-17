#ifndef EXECUTE_H
#define EXECUTE_H

#include <stdbool.h>

#define EC_STDERR_APPEND 1
#define EC_STDOUT_APPEND (1 << 1)

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

int execute(const execution_context context);

#endif
