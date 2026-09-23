#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtin_funcs.h"
#include "readers/file_reader.h"
#include "interpreter/repl.h"
#include "lib/error.h"
#include "readers/generic_reader.h"

static const char *const SOURCE_HELP =
    "Usage: source FILENAME\n"
    "Read and execute the contents of FILENAME, this will modify the current\n"
    "state of the shell (shell variables, jobs, etc.).";

int builtin_source(char **argv) {
  static int recursion_count = 0;

  if (argv[1] == NULL) {
    error_f("%s\n", SOURCE_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(SOURCE_HELP);
    return EXIT_SUCCESS;
  }

  GenericReader file_reader;
  
  if (file_reader_create(&file_reader, argv[1])) {
    error_f("source: %s: %s\n", argv[1], strerror(errno));
    return EXIT_FAILURE;
  }

  if (recursion_count > 10000) {
    recursion_count = 0;
    error_f("rash: source... source... source... souce...\n");
    return EXIT_FAILURE;
  }

  recursion_count++;

  int status_code = repl(&file_reader);
  generic_reader_destroy(&file_reader);

  recursion_count--;

  return status_code;
}
