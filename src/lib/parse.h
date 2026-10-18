#ifndef LIB_PARSE_H
#define LIB_PARSE_H

#include "optional.h"

typedef OPTIONAL(int) OptionInt;

OptionInt parse_int(const char *str);

typedef OPTIONAL(unsigned int) OptionUnsignedInt;

OptionUnsignedInt parse_unsigned_int(const char *str);

typedef OPTIONAL(long) OptionLong;

OptionLong parse_long(const char *str);

typedef OPTIONAL(unsigned long) OptionUnsignedLong;

OptionUnsignedLong parse_unsigned_long(const char *str);

typedef OPTIONAL(double) OptionDouble;

OptionDouble parse_double(const char *str);

#endif
