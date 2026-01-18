#include "lib/f_error.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

__attribute__((__format__(__printf__, 1, 2))) 
void f_error(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  int res = vfprintf(stderr, format, ap);
  va_end(ap);

  assert(res != -1);
}
