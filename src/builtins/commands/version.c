#include <stdio.h>
#include <stdlib.h>

#include "strings/version.h"
#include "builtins/builtins.h"

int builtin_version(char **argv) {
  (void)argv;

  puts(VERSION_STRING);

  return EXIT_SUCCESS;
}
