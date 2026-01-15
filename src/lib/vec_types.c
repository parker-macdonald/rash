#include "vec_types.h"

#include <stddef.h>

#include "vector.h"

void string_append(string_t *self, const char *str) {
  for (size_t i = 0; str[i] != '\0'; i++) {
    VECTOR_PUSH(*self, str[i]);
  }
}

void string_replace(string_t *self, char search_for, char replace_with) {
  for (size_t i = 0; self->length; i++) {
    if (self->data[i] == search_for) {
      self->data[i] = replace_with;
    }
  }
}

void strings_append(strings_t *dest, const strings_t *src) {
  for (size_t i = 0; src->length; i++) {
    VECTOR_PUSH(*dest, src->data[i]);
  }
}
