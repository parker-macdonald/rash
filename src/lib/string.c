#include "lib/string.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "lib/next_pow_2.h"
#include "vector.h"

void string_append_ptr(String *self, const char *data, size_t length) {
  size_t new_length = length + self->length;

  if (self->_capacity < new_length) {
    self->_capacity = next_pow_2(new_length);
    char *new_data = realloc(self->data, self->_capacity);
    if (new_data == NULL) {
      abort();
    }
    self->data = new_data;
  }

  char *line_end = self->data + self->length;

  memcpy(line_end, data, length);

  self->length = new_length;
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
  for (size_t i = 0; i < self->length; i++) {
    if (self->data[i] == search_for) {
      self->data[i] = replace_with;
    }
  }
}

bool string_contains(const String *self, char search_for) {
  for (size_t i = 0; i < self->length; i++) {
    if (self->data[i] == search_for) {
      return true;
    }
  }

  return false;
}

size_t string_index_of(const String *self, char search_for) {
  for (size_t i = 0; i < self->length; i++) {
    if (self->data[i] == search_for) {
      return i;
    }
  }

  return (size_t)-1;
}

size_t string_last_index_of(const String *self, char search_for) {
  size_t location = (size_t)-1;

  for (size_t i = 0; i < self->length; i++) {
    if (self->data[i] == search_for) {
      location = i;
    }
  }

  return location;
}

String string_clone(const String *other) {
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

String string_substring(const String *self, size_t begin, size_t end) {
  return string_from_ptr(self->data + begin, end - begin);
}

void string_destroy(String *self) {
  VECTOR_DESTROY(*self);
}

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

int string_compare(const String *s1, const String *s2) {
  size_t min_length = s1->length > s2->length ? s2->length : s1->length;

  for (size_t i = 0; i < min_length; i++) {
    int diff = s1->data[i] - s2->data[i];

    if (diff != 0) {
      return diff;
    }
  }

  if (s1->length < s2->length) {
    return -1;
  }
  if (s1->length > s2->length) {
    return 1;
  }

  return 0;
}

void stringlist_destroy(StringList *list) {
  for (size_t i = 0; i < list->length; i++) {
    string_destroy(&list->data[i]);
  }

  VECTOR_DESTROY(*list);
}
