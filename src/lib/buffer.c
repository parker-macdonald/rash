#include "lib/buffer.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lib/next_pow_2.h"
#include "lib/vector.h"

void buffer_append_string(Buffer *self, const char *str) {
  // if capacity is -1, this buffer is read only
  assert(self->_capacity != (size_t)-1);

  for (size_t i = 0; str[i] != '\0'; i++) {
    VECTOR_PUSH(*self, (uint8_t)str[i]);
  }
}

void buffer_copy(Buffer *dest, const Buffer *src) {
  // if capacity is -1, this buffer is read only
  assert(dest->_capacity != (size_t)-1);

  if (dest->_capacity < src->length) {
    free(dest->data);
    dest->_capacity = next_pow_2(src->length);
    dest->data = malloc(dest->_capacity);
  }

  dest->length = src->length;
  memcpy(dest->data, src->data, src->length);
}

void buffer_insert(Buffer *buffer, size_t buffer_offset, uint8_t byte) {
  // if capacity is -1, this buffer is read only
  assert(buffer->_capacity != (size_t)-1);

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

void buffer_insert_bulk(
    Buffer *buffer, const uint8_t *src, size_t src_len, size_t buffer_offset
) {
  // if capacity is -1, this buffer is read only
  assert(buffer->_capacity != (size_t)-1);

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

    memcpy(line_end, src, src_len);

    buffer->length = new_length;
    return;
  }

  uint8_t *offset = buffer->data + buffer_offset;

  // move previous data over
  // i think this is the first feature newer than c99 i've used so far (besides
  // static_assert)
  memmove(offset + src_len, offset, buffer->length - buffer_offset);

  // copy new data in
  memcpy(offset, src, src_len);

  buffer->length = new_length;
}

void buffer_remove_bulk(Buffer *buffer, size_t offset, size_t count) {
  buffer->length -= count;

  if (buffer->length == offset + count) {
    return;
  }

  memmove(buffer->data, buffer->data + count, buffer->length);
}

Buffer buffer_from(const uint8_t *data, size_t length) {
  Buffer buffer;

  buffer._capacity = next_pow_2(length);
  buffer.data = malloc(buffer._capacity);
  buffer.length = length;

  memcpy(buffer.data, data, length);

  return buffer;
}

Buffer buffer_using(const uint8_t *data, size_t length) {
  Buffer buffer;

  buffer._capacity = (size_t)-1;
  // scary cast that discards const
  buffer.data = (uint8_t *)data;
  buffer.length = length;

  return buffer;
}
