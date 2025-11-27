#include <stdio.h>
#include <stdlib.h>

#include "../builtins.h"

const char *const VERSION_STRING = "rash version: "
#ifdef VERSION
    VERSION
#else
                                   "no one knows"
#endif
                                   "\ncompiled on: "
#ifdef __DATE__
    __DATE__
#else
                                   "who knows when"
#endif
    ;

int builtin_version(char **argv) {
  (void)argv;

  puts(VERSION_STRING);

  return EXIT_SUCCESS;
}
