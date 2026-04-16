#include "lib/string.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "vector.h"

void string_append(String *self, const char *str) {
  for (size_t i = 0; str[i] != '\0'; i++) {
    VECTOR_PUSH(*self, str[i]);
  }
}

void string_replace(String *self, char search_for, char replace_with) {
  for (size_t i = 0; self->length; i++) {
    if (self->data[i] == search_for) {
      self->data[i] = replace_with;
    }
  }
}

void strings_append(StringList *dest, const StringList *src) {
  for (size_t i = 0; src->length; i++) {
    VECTOR_PUSH(*dest, src->data[i]);
  }
}
