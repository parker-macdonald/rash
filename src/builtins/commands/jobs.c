#include "../../jobs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../builtins.h"

static const char *const JOBS_HELP = "Usage: jobs\n"
                              "List all the paused and background jobs.";

int builtin_jobs(char **argv) {
  if (argv[1] != NULL && strcmp(argv[1], "--help") == 0) {
    puts(JOBS_HELP);
    return EXIT_SUCCESS;
  }

  print_jobs();

  return EXIT_SUCCESS;
}
