#include "utf_8.h"

#include <stddef.h>
#include <stdint.h>

#include "lib/buffer.h"

#define LAST_BIT_OF_BYTE_MASK 0x80

static unsigned int count_leading_ones(uint8_t byte) {
  unsigned int count;

  for (count = 0; byte & LAST_BIT_OF_BYTE_MASK; count++) {
    byte <<= 1;
  }

  return count;
}

size_t utf8_prev_codepoint(const Buffer *buffer, size_t buffer_offset) {
  if (buffer_offset == 0) {
    return 0;
  }

  size_t char_size = 1;
  size_t start = buffer_offset;

  buffer_offset--;

  // true until we've reached the start of the utf-8 char
  while (is_continuation_byte_utf8(buffer->data[buffer_offset])) {
    // if we've reached the start this is malformed utf-8, just treat the bad
    // character as a byte. also, char size should not be four in this loop,
    // if it is the data is malformed, again just treat the bad character as
    // a byte
    if (buffer_offset == 0 || char_size == 4) {
      return start - 1;
    }

    buffer_offset--;
    char_size++;
  }

  // checks to make sure the starting utf-8 byte we found is valid, if it
  // isn't, the data is malformed and tread the initial bad character as a
  // byte.
  if (count_leading_ones(buffer->data[buffer_offset]) != char_size) {
    return start - 1;
  }

  return buffer_offset;
}

size_t utf8_codepoint_size(const Buffer *buffer, size_t buffer_offset) {
  if (buffer_offset >= buffer->length) {
    return 0;
  }

  size_t start = buffer_offset;
  size_t char_size = (size_t)count_leading_ones(buffer->data[buffer_offset]);

  // if char_size is has a bad number of leading zeros, treat the character as
  // a byte
  switch (char_size) {
    case 2:
    case 3:
    case 4:
      break;
    default:
      return 1;
  }

  for (buffer_offset++; buffer_offset - start < char_size; buffer_offset++) {
    // if we go outside the buffer, the utf-8 is malformed, just treat the
    // character as a byte
    if (buffer_offset >= buffer->length) {
      return 1;
    }

    // checks if the continue sequence exists in the next character, if it
    // doesn't this utf-8 is malformed and we're treating the character as a
    // byte
    if (!is_continuation_byte_utf8(buffer->data[buffer_offset])) {
      return 1;
    }
  }

  return char_size;
}

size_t utf8_next_codepoint(const Buffer *buffer, size_t buffer_offset) {
  return utf8_codepoint_size(buffer, buffer_offset) + buffer_offset;
}

unsigned utf8_count_codepoint(const Buffer *buffer) {
  unsigned length = 0;
  size_t i = 0;

  while (i < buffer->length) {
    i = utf8_next_codepoint(buffer, i);
    length++;
  }

  return length;
}

bool is_continuation_byte_utf8(const uint8_t byte) {
  return (count_leading_ones(byte) == 1);
}

size_t utf8_remove_codepoint(Buffer *buffer, size_t buffer_offset) {
  size_t codepoint_size = utf8_codepoint_size(buffer, buffer_offset);

  buffer_remove_bulk(buffer, buffer_offset, codepoint_size);

  return codepoint_size;
}
