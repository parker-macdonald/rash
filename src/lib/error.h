#ifndef ERROR_H
#define ERROR_H

#include <errno.h>

#include "lib/attrib.h"

void error(const char *str);

ATTRIB_PRINTF(1, 2)
void error_f(const char *restrict format, ...);

ATTRIB_NORETURN
void fatal(const char *str);

ATTRIB_NORETURN
ATTRIB_PRINTF(1, 2)
void fatal_f(const char *restrict format, ...);

#define rash_assert(expr, str)                                                 \
  do {                                                                         \
    if (expr)                                                                  \
      fatal_f("%s:%d: %s\n", __FILE__, __LINE__, str);                         \
  } while (0)

ATTRIB_NORETURN
void rash_panic(void);

#define unreachable() fatal_f("%s:%d: reached unreachable code\n", __FILE__, __LINE__)

#endif
