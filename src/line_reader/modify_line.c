#include "modify_line.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "../utf_8.h"
#include "../vector.h"
#include "line_reader.h"

void line_insert(line_t *const line, const uint8_t byte,
                 const size_t cursor_pos) {
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

size_t line_backspace(line_t *const line, const size_t cursor_pos) {
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

size_t line_delete(line_t *const line, const size_t cursor_pos) {
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

void line_copy(line_t *dest, const line_t *const src) {
  VECTOR_CLEAR(*dest);

  for (size_t i = 0; i < src->length; i++) {
    VECTOR_PUSH(*dest, src->data[i]);
  }
}
