#include "lib/error.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib/attrib.h"

void error(const char* str) {
  int res = fputs(str, stderr);

  assert(res != EOF);
}

ATTRIB_PRINTF(1, 2) 
void error_f(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  int res = vfprintf(stderr, format, ap);
  va_end(ap);

  assert(res != -1);
}

ATTRIB_NORETURN
void fatal(const char* str) {
  int res = fputs(str, stderr);

  assert(res != EOF);

  exit(EXIT_FAILURE);
}

ATTRIB_NORETURN
ATTRIB_PRINTF(1, 2)
void fatal_f(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  int res = vfprintf(stderr, format, ap);
  va_end(ap);

  assert(res != -1);

  exit(EXIT_FAILURE);
}
