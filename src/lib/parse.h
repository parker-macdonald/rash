#ifndef LIB_PARSE_H
#define LIB_PARSE_H

#include "optional.h"

typedef OPTIONAL(int) OptionInt;

OptionInt parse_int(const char *str);

typedef OPTIONAL(double) OptionDouble;

OptionDouble parse_double(const char *str);

#endif
