#include "modify_line.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../utf_8.h"
#include "../vector.h"

void line_insert(
    buf_t *const line, const uint8_t byte, const size_t cursor_pos
) {
  if (cursor_pos == line->length) {
    VECTOR_PUSH((*line), byte);
    return;
  }

  uint8_t old = line->data[cursor_pos];
  line->data[cursor_pos] = byte;

  for (size_t i = cursor_pos + 1; i < line->length; i++) {
    const uint8_t old2 = line->data[i];

    line->data[i] = old;

    old = old2;
  }

  VECTOR_PUSH((*line), old);
}

size_t line_backspace(buf_t *const line, const size_t cursor_pos) {
  const size_t char_size = traverse_back_utf8(line->data, cursor_pos);

  if (line->length == cursor_pos) {
    line->length -= char_size;
    return char_size;
  }

  const size_t offset = cursor_pos - char_size;
  line->length -= char_size;

  for (size_t i = offset; i < line->length; i++) {
    line->data[i] = line->data[i + char_size];
  }

  return char_size;
}

size_t line_delete(buf_t *const line, const size_t cursor_pos) {
  const size_t char_size =
      traverse_forward_utf8(line->data, line->length, cursor_pos);

  if (line->length == cursor_pos + char_size) {
    line->length -= char_size;
    return char_size;
  }

  line->length -= char_size;

  for (size_t i = cursor_pos; i < line->length; i++) {
    line->data[i] = line->data[i + char_size];
  }

  return char_size;
}

void line_copy(buf_t *dest, const buf_t *const src) {
  VECTOR_CLEAR(*dest);

  for (size_t i = 0; i < src->length; i++) {
    VECTOR_PUSH(*dest, src->data[i]);
  }
}

// calculate the next power of 2 given a number n, this is optimized for 64 bit
// and 32 bit cpus, before falling back onto a generic implementation
static inline size_t next_pow_2(size_t n) {
  if (sizeof(size_t) == 8) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
  }
  if (sizeof(size_t) == 4) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
  }
  size_t p = 1;
  while (p < n) {
    p *= 2;
  }
  return p;
}

void line_insert_bulk(
    buf_t *line, size_t cursor_pos, uint8_t *src, size_t src_len
) {
  size_t new_length = src_len + line->length;

  if (line->_capacity < new_length) {
    line->_capacity = next_pow_2(new_length);
    line->data = realloc(line->data, line->_capacity);
  }

  if (cursor_pos == line->length) {
    uint8_t *line_end = line->data + line->length;

    for (size_t i = 0; i < src_len; i++) {
      *line_end = src[i];
      line_end++;
    }

    line->length = new_length;
    return;
  }

  // move previous data over
  // i think this is the first feature newer than c99 i've used so far (besides
  // static_assert)
  memmove(
      line->data + cursor_pos + src_len,
      line->data + cursor_pos,
      line->length - cursor_pos
  );

  // copy new data in
  for (size_t i = 0; i < src_len; i++) {
    line->data[i + cursor_pos] = src[i];
  }

  line->length = new_length;
}
