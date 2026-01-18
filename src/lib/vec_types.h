#ifndef VEC_TYPES_H
#define VEC_TYPES_H

#include <stdint.h>

#include "vector.h"

typedef VECTOR(uint8_t) buf_t;

void buf_append_string(buf_t *self, const char *str);

typedef VECTOR(char) string_t;

void string_append(string_t *self, const char *str);

void string_replace(string_t *self, char search_for, char replace_with);

typedef VECTOR(char *) strings_t;

void strings_append(strings_t *dest, const strings_t *src);

#endif
