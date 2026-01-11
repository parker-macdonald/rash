#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../file_reader.h"
#include "../../interpreter/repl.h"
#include "builtins/builtins.h"

static const char *const SOURCE_HELP =
    "Usage: source FILENAME\n"
    "Read and execute the contents of FILENAME, this will modify the current\n"
    "state of the shell (shell variables, jobs, etc.).";

int builtin_source(char **argv) {
  if (argv[1] == NULL) {
    (void)fprintf(stderr, "%s\n", SOURCE_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(SOURCE_HELP);
    return EXIT_SUCCESS;
  }

  FILE *file = fopen(argv[1], "r");

  if (file == NULL) {
    (void)fprintf(stderr, "source: %s: %s\n", argv[1], strerror(errno));
    return EXIT_FAILURE;
  }

  struct file_reader reader_data;
  file_reader_init(&reader_data, file);

  return repl(file_reader_read, &reader_data);
}
