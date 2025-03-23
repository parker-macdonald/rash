#include <stdlib.h>
#include <stdio.h>

int builtin_help(char **const argv) {
  (void)argv;
  
  puts("Welcome to rash, the rat ass shell!");

  return EXIT_SUCCESS;
}
