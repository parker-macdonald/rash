#include "utf_8.h"
#include <stddef.h>

unsigned int traverse_back_utf8(const char *const line,
                                const unsigned int cursor_pos) {
  unsigned int offset = cursor_pos - 1;
  unsigned int char_size = 1;

  // true until we've reached the start of the utf-8 char
  while (~line[offset] & 0xc0) {
    // if we've reached the start this is malformed utf-8, just treat the bad
    // character as a byte. also, char size should not be four in this loop, if
    // it is the data is malformed, again just treat the bad character as a byte
    if (offset == 0 || char_size == 4) {
      char_size = 1;
      offset = cursor_pos - 1;
      break;
    }

    offset--;
    char_size++;
  }

  // checks to make sure the starting utf-8 byte we found is valid, if it isn't,
  // the data is malformed and tread the initial bad character as a byte.
  switch (char_size) {
  case 4:
    if ((line[offset] & 0xf8) != 0xf0) {
      char_size = 1;
      break;
    }
    break;
  case 3:
    if ((line[offset] & 0xf0) != 0xe0) {
      char_size = 1;
      break;
    }
    break;
  case 2:
    if ((line[offset] & 0xe0) != 0xc0) {
      char_size = 1;
      break;
    }
    break;
  }

  return char_size;
}

unsigned int traverse_forward_utf8(const char *const line,
                                   const size_t line_len,
                                   const unsigned int cursor_pos) {
  unsigned int offset = cursor_pos;
  unsigned int char_size = 1;

  // checks for stating sequence of a two byte utf-8 character
  if ((line[offset] & 0xe0) == 0xc0) {
    char_size = 2;
  }

  // checks for stating sequence of a three byte utf-8 character
  if ((line[offset] & 0xf0) == 0xe0) {
    char_size = 3;
  }

  // checks for stating sequence of a four byte utf-8 character
  if ((line[offset] & 0xf8) == 0xf0) {
    char_size = 4;
  }

  for (unsigned int i = offset; i < char_size; i++) {
    // if we go outside the buffer, the utf-8 is malformed, just treat the
    // character as a byte
    if (i >= line_len) {
      return 1;
    }

    // checks if the continue sequence exists in the next character, if it
    // doesn't this utf-8 is malformed and we're treating the character as a
    // byte
    if (!(~line[offset] & 0xc0)) {
      return 1;
    }
  }

  return char_size;
}

bool is_continuation_byte_utf8(const char c) {
  return ((c & 0xc0) == 0x80);
}
