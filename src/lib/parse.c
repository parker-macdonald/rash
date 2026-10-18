#include "parse.h"
#include <limits.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

OptionLong parse_long(const char *str) {
  char *endptr;
  errno = 0;
  long num = strtol(str, &endptr, 10);

  if (errno != 0 || *endptr != '\0') {
    return (OptionLong){.has_value = false};
  }

  return (OptionLong){.has_value = true, .value = num};
}

OptionInt parse_int(const char *str) {
  OptionLong num = parse_long(str);

  if (!num.has_value || num.value < INT_MIN || num.value > INT_MAX) {
    return (OptionInt){.has_value = false};
  }

  return (OptionInt){.has_value = true, .value = (int)num.value};
}

OptionUnsignedLong parse_unsigned_long(const char *str) {
  char *endptr;
  errno = 0;
  unsigned long num = strtoul(str, &endptr, 10);

  if (errno != 0 || *endptr != '\0') {
    return (OptionUnsignedLong){ .has_value = false };
  }

  return (OptionUnsignedLong){ .has_value = true, .value = num };
}

OptionUnsignedInt parse_unsigned_int(const char *str) {
  OptionUnsignedLong num = parse_unsigned_long(str);

  if (!num.has_value || num.value > UINT_MAX) {
    return (OptionUnsignedInt){.has_value = false};
  }

  return (OptionUnsignedInt){.has_value = true, .value = (unsigned)num.value};
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
