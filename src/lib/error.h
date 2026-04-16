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

#endif
