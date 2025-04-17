#include "../../jobs.h"
#include "../builtins.h"
#include <stdlib.h>

int builtin_jobs(char **argv) {
  (void)argv;
  print_jobs();

  return EXIT_SUCCESS;
}
