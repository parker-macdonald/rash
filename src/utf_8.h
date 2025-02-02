#ifndef UTF_8_H
#define UTF_8_H

#include <stddef.h>

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

#endif
