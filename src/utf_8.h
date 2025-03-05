#ifndef UTF_8_H
#define UTF_8_H

#include <stdbool.h>
#include <stddef.h>

#define UTF_8_CONTINUATION_BIT_MASK 0xc0
#define UTF_8_CONTINUATION_SEQUENCE 0x80

#define UTF_8_FOUR_BYTE_MASK 0xf8
#define UTF_8_FOUR_BYTE_SEQUENCE 0xf0

#define UTF_8_THREE_BYTE_MASK 0xf0
#define UTF_8_THREE_BYTE_SEQUENCE 0xe0

#define UTF_8_TWO_BYTE_MASK 0xe0
#define UTF_8_TWO_BYTE_SEQUENCE 0xc0

/**
 * @brief When given a line, traverse_back_utf8 will backtrack to find the
 * starting byte and then return the length of the character in bytes.
 * @param line The line to traverse.
 * @param cursor_pos The position of the cursor, traverse_back_utf8 will start
 * backtracking at the byte before the cursor.
 * @return The number of bytes the character takes up, or one if string is not
 * valid utf-8.
 */
unsigned int traverse_back_utf8(const char *const line,
                                const unsigned int cursor_pos);

/**
 * @brief When given a line, traverse_forward_utf8 will traverse through the
 * line to find the ending byte and then return the length of the character in
 * bytes.
 * @param line The line to traverse.
 * @param cursor_pos The position of the cursor.
 * @return The number of bytes the character takes up, or one if string is not
 * valid utf-8.
 */
unsigned int traverse_forward_utf8(const char *const line,
                                   const size_t line_len,
                                   const unsigned int cursor_pos);

/**
 * @brief Returns true when the given character is a continuation byte in utf-8.
 * @param c The byte to check.
 * @return true: byte is a continuation byte, false: it is not.
 */
bool is_continuation_byte_utf8(const char c);

#endif
