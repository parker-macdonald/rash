#ifndef VEC_TYPES_H
#define VEC_TYPES_H

#include <stdint.h>

#include "vector.h"

typedef VECTOR(uint8_t) Buffer;

void buffer_append_string(Buffer *self, const char *str);

void buffer_copy(Buffer *dest, const Buffer *src);

void buffer_insert(Buffer *buffer, size_t buffer_offset, uint8_t byte);

void buffer_insert_bulk(Buffer *buffer, const uint8_t *src, size_t src_len, size_t buffer_offset);

typedef VECTOR(char) String;

void string_append(String *self, const char *str);

void string_replace(String *self, char search_for, char replace_with);

typedef VECTOR(char *) StringList;

void strings_append(StringList *dest, const StringList *src);

#endif
