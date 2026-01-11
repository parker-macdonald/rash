#ifndef VEC_TYPES_H
#define VEC_TYPES_H

#include <stdint.h>

#include "vector.h"

typedef VECTOR(char) string_t;

typedef VECTOR(uint8_t) buf_t;

typedef VECTOR(char *) strings_t;

#endif
