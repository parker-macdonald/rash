#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtins.h"
#include "file_reader.h"
#include "interpreter/repl.h"
#include "lib/f_error.h"

static const char *const SOURCE_HELP =
    "Usage: source FILENAME\n"
    "Read and execute the contents of FILENAME, this will modify the current\n"
    "state of the shell (shell variables, jobs, etc.).";

int builtin_source(char **argv) {
  static int recursion_count = 0;

  if (argv[1] == NULL) {
    f_error("%s\n", SOURCE_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(SOURCE_HELP);
    return EXIT_SUCCESS;
  }

  FILE *file = fopen(argv[1], "r");

  if (file == NULL) {
    f_error("source: %s: %s\n", argv[1], strerror(errno));
    return EXIT_FAILURE;
  }

  struct file_reader reader_data;
  file_reader_init(&reader_data, file);

  if (recursion_count > 10000) {
    f_error("rash: source... source... source... souce...\n");
    return EXIT_FAILURE;
  }

  recursion_count++;

  int status_code = repl(file_reader_read, &reader_data);

  recursion_count--;

  return status_code;
}
