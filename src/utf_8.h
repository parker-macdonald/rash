#ifndef UTF_8_H
#define UTF_8_H

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

#endif
