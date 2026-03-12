#include "dynamic_sprintf.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

__attribute__((__format__(__printf__, 1, 2)))
char *dynamic_sprintf(const char *restrict format, ...) {
  char *buffer;
  size_t size;

  FILE *stream = open_memstream(&buffer, &size);

  if (stream == NULL) {
    return NULL;
  }

  va_list ap;
  va_start(ap, format);
  
  int result = vfprintf(stream, format, ap);

  va_end(ap);

  if (result == -1) {
    int saved_errno = errno;

    if (fclose(stream) != 0) {
      errno = saved_errno;
    }
    free(buffer);
    return NULL;
  }

  if (fclose(stream) != 0) {
    free(buffer);
    return NULL;
  }

  return buffer;
}
