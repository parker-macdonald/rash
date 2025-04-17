#include "../../jobs.h"

#include <stdlib.h>

#include "../builtins.h"

int builtin_jobs(char **argv) {
  (void)argv;
  print_jobs();

  return EXIT_SUCCESS;
}
