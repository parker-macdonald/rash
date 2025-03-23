#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

extern bool should_exit;

#define BASE 10

int builtin_exit(char **const argv) {
  should_exit = true;

  if (argv[1] == NULL) {
    return 0;
  }

  int old_errno = errno;
  long status = strtol(argv[1], NULL, BASE);

  if (old_errno != errno) {
    perror("exit");
    return 2;
  }

  return (int)status;
}
