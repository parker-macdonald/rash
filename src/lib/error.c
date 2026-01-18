#include "lib/error.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void error(const char* str) {
  int res = fputs(str, stderr);

  assert(res != EOF);
}

__attribute__((__format__(__printf__, 1, 2))) 
void error_f(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  int res = vfprintf(stderr, format, ap);
  va_end(ap);

  assert(res != -1);
}

void fatal(const char* str) {
  int res = fputs(str, stderr);

  assert(res != EOF);

  exit(EXIT_FAILURE);
}

__attribute__((__format__(__printf__, 1, 2)))
void fatal_f(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  int res = vfprintf(stderr, format, ap);
  va_end(ap);

  assert(res != -1);

  exit(EXIT_FAILURE);
}
