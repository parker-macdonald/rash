#include <stdio.h>
#include <stdlib.h>

int builtin_version(char **argv) {
  (void)argv;

  printf(
      "rash version: %s\ncompiled on: %s\n",
#ifdef VERSION
      VERSION
#else
      "no one knows"
#endif
      ,
#ifdef __DATE__
      __DATE__
#else
      "who knows when"
#endif
  );

  return EXIT_SUCCESS;
}
