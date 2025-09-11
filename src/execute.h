#ifndef EXECUTE_H
#define EXECUTE_H

#include <stdbool.h>

#define EC_STDERR_APPEND 1
#define EC_STDOUT_APPEND (1 << 1)

typedef struct {
  // arguments for command
  char **argv;
  // path to redirect stdout, or null
  char *stdout;
  // path to redirect stdin, or null
  char *stdin;
  // path to redirect stderr, or null
  char *stderr;
  int flags;
} execution_context;

int execute(const execution_context context);

#endif
