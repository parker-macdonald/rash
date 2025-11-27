#ifndef UTF_8_H
#define UTF_8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief When given a line, traverse_back_utf8 will backtrack to find the
 * starting byte and then return the length of the character in bytes.
 * @param line The line to traverse.
 * @param cursor_pos The position of the cursor, traverse_back_utf8 will start
 * backtracking at the byte before the cursor.
 * @return The number of bytes the character takes up, or one if string is not
 * valid utf-8.
 */
size_t traverse_back_utf8(const uint8_t *const line, const size_t cursor_pos);

/**
 * @brief When given a line, traverse_forward_utf8 will traverse through the
 * line to find the ending byte and then return the length of the character in
 * bytes.
 * @param line The line to traverse.
 * @param cursor_pos The position of the cursor.
 * @return The number of bytes the character takes up, or one if string is not
 * valid utf-8.
 */
size_t traverse_forward_utf8(
    const uint8_t *const line, const size_t line_len, const size_t cursor_pos
);

/**
 * @brief Returns true when the given character is a continuation byte in utf-8.
 * @param byte The byte to check.
 * @return true: byte is a continuation byte, false: it is not.
 */
bool is_continuation_byte_utf8(const uint8_t byte);

/**
 * @brief returns the number of characters in a string that is utf-8 encoded.
 * @param str the string to read.
 * @param len the length of the string in bytes.
 * @return the number of characters in the string.
 */
size_t strlen_utf8(const uint8_t *const str, size_t len);

#endif
