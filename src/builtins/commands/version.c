#include <stdio.h>
#include <stdlib.h>

#include "../builtins.h"

int builtin_version(char **argv) {
  (void)argv;

  puts(
      "rash version: "
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
  );

  return EXIT_SUCCESS;
}
