#include "dynamic_sprintf.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

__attribute__((__format__ (__printf__, 1, 2)))
char *dynamic_sprintf(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  va_list ap2;
  va_copy(ap2, ap);
  int size = vsnprintf(NULL, 0, format, ap2);
  va_end(ap2);

  if (size == -1) {
    va_end(ap);
    return NULL;
  }

  char *buffer = malloc((size_t)size + 1);

  int new_size = vsnprintf(buffer, (size_t)size + 1, format, ap);
  va_end(ap);

  if (size != new_size) {
    free(buffer);
    return NULL;
  }

  return buffer;
}
