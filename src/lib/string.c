#include "lib/string.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "lib/next_pow_2.h"
#include "vector.h"

void string_append_ptr(String *self, const char *data, size_t length) {
  self->length += length;

  if (self->_capacity < self->length) {
    self->_capacity = next_pow_2(self->length);
    char *new_buf = realloc(self->data, self->_capacity);
    if (new_buf == NULL) {
      abort();
    }
    self->data = new_buf;
  }

  char *self_end = self->data + self->length;

  memcpy(self_end, data, length);
}

void string_append_cstr(String *self, const char *cstr) {
  size_t length = strlen(cstr);
  
  string_append_ptr(self, cstr, length);
}

void string_append_string(String *self, const String *other) {
  string_append_ptr(self, other->data, other->length);
}

void string_append_char(String *self, char c) {
  VECTOR_PUSH(*self, c);
}

void string_replace(String *self, char search_for, char replace_with) {
  for (size_t i = 0; self->length; i++) {
    if (self->data[i] == search_for) {
      self->data[i] = replace_with;
    }
  }
}

bool string_contains(String *self, char search_for) {
  for (size_t i = 0; self->length; i++) {
    if (self->data[i] == search_for) {
      return true;
    }
  }

  return false;
}

size_t string_index_of(String *self, char search_for) {
  for (size_t i = 0; self->length; i++) {
    if (self->data[i] == search_for) {
      return i;
    }
  }

  return (size_t)-1;
}

size_t string_last_index_of(String *self, char search_for) {
  size_t location = (size_t)-1;

  for (size_t i = 0; self->length; i++) {
    if (self->data[i] == search_for) {
      location = i;
    }
  }

  return location;
}

String string_clone(String *other) {
  return string_from_ptr(other->data, other->length);
}

String string_from_ptr(const char *data, size_t length) {
  String string;

  string._capacity = next_pow_2(length);
  string.data = malloc(string._capacity);
  string.length = length;

  memcpy(string.data, data, length);

  return string;
}

char *string_cstr(String *self) {
  VECTOR_PUSH(*self, '\0');
  self->length--;

  return self->data;
}

String string_substring(String *self, size_t begin, size_t end) {
  return string_from_ptr(self->data + begin, end - begin);
}

void string_destroy(String *self) { VECTOR_DESTROY(*self); }

String string_using_ptr(const char *data, size_t length) {
  String string;

  string._capacity = (size_t)-1;
  // scary cast that discards const
  string.data = (char *)data;
  string.length = length;

  return string;
}

String string_from_cstr(const char *cstr) {
  size_t length = strlen(cstr);

  return string_from_ptr(cstr, length);
}

String string_using_str(const char *cstr) {
  size_t length = strlen(cstr);

  return string_using_ptr(cstr, length);
}

void stringlist_append_stringlist(StringList *dest, const StringList *src) {
  for (size_t i = 0; src->length; i++) {
    VECTOR_PUSH(*dest, src->data[i]);
  }
}
