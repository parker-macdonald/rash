#ifndef ERROR_H
#define ERROR_H

#include "lib/attrib.h"

void error(const char *str);

ATTRIB_PRINTF(1, 2)
void error_f(const char *restrict format, ...);

ATTRIB_NORETURN
void fatal(const char *str);

ATTRIB_NORETURN
ATTRIB_PRINTF(1, 2)
void fatal_f(const char *restrict format, ...);

void rash_unwind(void);

#define rash_panic()                                                           \
  do {                                                                         \
    ATTRIB_NORETURN extern void _exit(int status);                             \
    error_f("rash has panicked at %s:%d\n", __FILE__, __LINE__);               \
    rash_unwind();                                                             \
    _exit(1);                                                                  \
  } while (0)

#define rash_assert(expr, str)                                                 \
  do {                                                                         \
    if (!(expr)) {                                                             \
      error("assertion failed: `" #expr "`, " str ".\n");                      \
      rash_panic();                                                            \
    }                                                                          \
  } while (0)

#define unreachable()                                                          \
  do {                                                                         \
    error("reached unreachable code.\n");                                      \
    rash_panic();                                                              \
  } while (0)

#endif
