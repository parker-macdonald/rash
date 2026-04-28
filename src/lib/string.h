#ifndef VEC_TYPES_H
#define VEC_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "vector.h"

typedef VECTOR(char) String;

void string_append_cstr(String *self, const char *cstr);

void string_append_string(String *self, const String *other);

void string_append_ptr(String *self, const char *data, size_t length);

void string_append_char(String *self, char c);

void string_replace(String *self, char search_for, char replace_with);

bool string_contains(String *self, char search_for);

size_t string_index_of(String *self, char search_for);

size_t string_last_index_of(String *self, char search_for);

String string_clone(String *other);

void string_destroy(String *self);

char *string_cstr(String *self);

String string_substring(String *self, size_t begin, size_t end);

// constructs a string by copying the data at `data` of length `length`. if this
// were rust i would say that the returned string owns it's data.
String string_from_ptr(const char *data, size_t length);

// constructs a string using the pointer `data` and the length `length`. if this
// were rust i would say the the returned string borrows it's data.
String string_using_ptr(const char *data, size_t length);

// constructs a string by copying the data at `cstr` of length `strlen(cstr)`.
// if this were rust i would say that the returned string owns it's data.
String string_from_cstr(const char *cstr);

// constructs a string using the pointer `cstr` of length `strlen(cstr)`. if
// this were rust i would say the the returned string borrows it's data.
String string_using_str(const char *cstr);

typedef VECTOR(char *) StringList;

void strings_append(StringList *dest, const StringList *src);

#endif
