#ifndef VEC_TYPES_H
#define VEC_TYPES_H

#include <stdint.h>

#include "vector.h"

typedef VECTOR(uint8_t) Buffer;

void buf_append_string(Buffer *self, const char *str);

void buf_copy(Buffer *dest, const Buffer *src);

typedef VECTOR(char) String;

void string_append(String *self, const char *str);

void string_replace(String *self, char search_for, char replace_with);

typedef VECTOR(char *) StringList;

void strings_append(StringList *dest, const StringList *src);

#endif
