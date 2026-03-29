#include "vec_types.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "lib/next_pow_2.h"
#include "vector.h"

void buffer_append_string(Buffer *self, const char *str) {
  for (size_t i = 0; str[i] != '\0'; i++) {
    VECTOR_PUSH(*self, (uint8_t)str[i]);
  }
}

void buffer_copy(Buffer *dest, const Buffer *src) {
  if (dest->_capacity > src->length) {
    free(dest->data);
    dest->data = malloc(next_pow_2(src->length));
  }

  dest->length = src->length;
  memcpy(dest->data, src->data, src->length);
}

void buffer_insert(Buffer *buffer, size_t buffer_offset, uint8_t byte) {
  if (buffer_offset >= buffer->length) {
    VECTOR_PUSH((*buffer), byte);
    return;
  }

  uint8_t old_value = buffer->data[buffer_offset];
  buffer->data[buffer_offset] = byte;

  for (size_t i = buffer_offset + 1; i < buffer->length; i++) {
    const uint8_t saved = buffer->data[i];

    buffer->data[i] = old_value;

    old_value = saved;
  }

  VECTOR_PUSH((*buffer), old_value);
}

void buffer_insert_bulk(Buffer *buffer, const uint8_t *src, size_t src_len, size_t buffer_offset) {
  size_t new_length = src_len + buffer->length;

  if (buffer->_capacity < new_length) {
    buffer->_capacity = next_pow_2(new_length);
    uint8_t *data = realloc(buffer->data, buffer->_capacity);
    if (data == NULL) {
      abort();
    }
    buffer->data = data;
  }

  if (buffer_offset == buffer->length) {
    uint8_t *line_end = buffer->data + buffer->length;

    for (size_t i = 0; i < src_len; i++) {
      *line_end = src[i];
      line_end++;
    }

    buffer->length = new_length;
    return;
  }

  // move previous data over
  // i think this is the first feature newer than c99 i've used so far (besides
  // static_assert)
  memmove(
      buffer->data + buffer_offset + src_len,
      buffer->data + buffer_offset,
      buffer->length - buffer_offset
  );

  // copy new data in
  for (size_t i = 0; i < src_len; i++) {
    buffer->data[i + buffer_offset] = src[i];
  }

  buffer->length = new_length;
}

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
