#include "utf_8.h"

#include <stddef.h>
#include <stdint.h>

#define LAST_BIT_OF_BYTE_MASK 0x80

static unsigned int count_leading_ones(uint8_t byte) {
  unsigned int count;

  for (count = 0; byte & LAST_BIT_OF_BYTE_MASK; count++) {
    byte <<= 1;
  }

  return count;
}

size_t traverse_back_utf8(const uint8_t *const line, const size_t cursor_pos) {
  size_t offset = cursor_pos - 1;
  size_t char_size = 1;

  // true until we've reached the start of the utf-8 char
  while (is_continuation_byte_utf8(line[offset])) {
    // if we've reached the start this is malformed utf-8, just treat the bad
    // character as a byte. also, char size should not be four in this loop,
    // if it is the data is malformed, again just treat the bad character as
    // a byte
    if (offset == 0 || char_size == 4) {
      return 1;
    }

    offset--;
    char_size++;
  }

  // checks to make sure the starting utf-8 byte we found is valid, if it
  // isn't, the data is malformed and tread the initial bad character as a
  // byte.
  if (count_leading_ones(line[offset]) != char_size) {
    char_size = 1;
  }

  return char_size;
}

size_t traverse_forward_utf8(
    const uint8_t *const line, const size_t line_len, const size_t cursor_pos
) {
  size_t offset = cursor_pos;
  size_t char_size = count_leading_ones(line[offset]);

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

  for (offset++; offset - cursor_pos < char_size; offset++) {
    // if we go outside the buffer, the utf-8 is malformed, just treat the
    // character as a byte
    if (offset >= line_len) {
      return 1;
    }

    // checks if the continue sequence exists in the next character, if it
    // doesn't this utf-8 is malformed and we're treating the character as a
    // byte
    if (!is_continuation_byte_utf8(line[offset])) {
      return 1;
    }
  }

  return char_size;
}

bool is_continuation_byte_utf8(const uint8_t byte) {
  return (count_leading_ones(byte) == 1);
}
