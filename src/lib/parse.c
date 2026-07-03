#include "parse.h"
#include <limits.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

OptionInt parse_int(const char *str) {
  char *endptr;
  errno = 0;
  long num = strtol(str, &endptr, 10);

  if (errno != 0 || *endptr != '\0' || num < INT_MIN || num > INT_MAX) {
    return (OptionInt){.has_value = false};
  }

  return (OptionInt){.has_value = true, .value = (int)num};
}

OptionDouble parse_double(const char *str) {
  char *endptr;
  errno = 0;
  double num = strtod(str, &endptr);

  if (errno != 0 || *endptr != '\0' || num < INT_MIN || num > INT_MAX) {
    return (OptionDouble){.has_value = false};
  }

  return (OptionDouble){.has_value = true, .value = num};
}
