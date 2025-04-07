#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 10

// returns true on error, false on success
bool get_nums_from_params(char **argv, long *p_a, long *p_b) {
  long a, b;
  char *endptr;
  errno = 0;
  a = strtol(argv[1], &endptr, BASE);

  if (errno != 0 || endptr == argv[1] || *endptr != '\0') {
    fprintf(stderr, "test: '%s' is not a number\n", argv[1]);
    return true;
  }

  b = strtol(argv[3], &endptr, BASE);
  if (errno != 0 || endptr == argv[3] || *endptr != '\0') {
    fprintf(stderr, "test: '%s' is not a number\n", argv[3]);
    return true;
  }

  *p_a = a;
  *p_b = b;

  return false;
}

int builtin_test(char **argv) {
  if (argv[1] == NULL) {
    return EXIT_FAILURE;
  }

  if (argv[1][0] == '-') {
    switch (argv[1][1]) {
    case 'n':
      if (argv[2] != NULL) {
        return strlen(argv[2]) == 0;
      }
      break;

    case 'z':
      if (argv[2] != NULL) {
        return strlen(argv[2]) != 0;
      }
      break;
    }
  }

  if (argv[2] != NULL) {
    if (argv[3] == NULL) {
      fprintf(stderr, "test: missing argument after '%s'\n", argv[2]);
      return EXIT_FAILURE;
    }

    if (strcmp(argv[2], "=") == 0) {
      return strcmp(argv[1], argv[3]) != 0;
    }

    if (strcmp(argv[2], "!=") == 0) {
      return strcmp(argv[1], argv[3]) == 0;
    }

    if (strcmp(argv[2], ">") == 0) {
      return !(strcoll(argv[1], argv[3]) > 0);
    }

    if (strcmp(argv[2], "<") == 0) {
      return !(strcoll(argv[1], argv[3]) < 0);
    }

    if (strcmp(argv[2], "-eq") == 0) {
      long a, b;

      if (get_nums_from_params(argv, &a, &b)) {
        return EXIT_FAILURE;
      }

      return !(a == b);
    }

    if (strcmp(argv[2], "-ge") == 0) {
      long a, b;

      if (get_nums_from_params(argv, &a, &b)) {
        return EXIT_FAILURE;
      }

      return !(a >= b);
    }

    if (strcmp(argv[2], "-gt") == 0) {
      long a, b;

      if (get_nums_from_params(argv, &a, &b)) {
        return EXIT_FAILURE;
      }

      return !(a > b);
    }

    if (strcmp(argv[2], "-le") == 0) {
      long a, b;

      if (get_nums_from_params(argv, &a, &b)) {
        return EXIT_FAILURE;
      }

      return !(a <= b);
    }

    if (strcmp(argv[2], "-lt") == 0) {
      long a, b;

      if (get_nums_from_params(argv, &a, &b)) {
        return EXIT_FAILURE;
      }

      return !(a < b);
    }

    if (strcmp(argv[2], "-ne") == 0) {
      long a, b;

      if (get_nums_from_params(argv, &a, &b)) {
        return EXIT_FAILURE;
      }

      return !(a != b);
    }

    fprintf(stderr, "test: invalid operator '%s'\n", argv[2]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
