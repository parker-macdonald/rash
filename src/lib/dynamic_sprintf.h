#ifndef DYNAMIC_SPRINTF_H
#define DYNAMIC_SPRINTF_H

#include "lib/attrib.h"

ATTRIB_PRINTF(1, 2)
char *dynamic_sprintf(const char *restrict format, ...);

#endif
