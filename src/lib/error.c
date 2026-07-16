#include "lib/error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/attrib.h"

void error(const char *str) {
  int res = fputs(str, stderr);

  if (res == EOF) {
    abort();
  }
}

ATTRIB_PRINTF(1, 2)
void error_f(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  int res = vfprintf(stderr, format, ap);
  va_end(ap);

  if (res == -1) {
    abort();
  }
}

ATTRIB_NORETURN
void fatal(const char *str) {
  (void)fputs(str, stderr);

  abort();
}

ATTRIB_NORETURN
ATTRIB_PRINTF(1, 2)
void fatal_f(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  (void)vfprintf(stderr, format, ap);
  va_end(ap);

  abort();
}
