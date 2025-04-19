#ifndef MODIFY_LINE_H
#define MODIFY_LINE_H

#include "line_reader.h"

/**
 * @brief adds the specified byte to the line at the cursor position
 * @param line the line to modify
 * @param byte the byte to add to the line
 * @param cursor_pos where the byte should go in the line
 */
void line_insert(line_t *const line, const uint8_t byte,
                 const size_t cursor_pos);

/**
 * @brief removes the utf-8 character behind the cursor position from the line
 * @param line the line to modify
 * @param cursor_pos current position of the cursor
 * @return the number of bytes removed from line
 */
size_t line_backspace(line_t *const line, const size_t cursor_pos);

/**
 * @brief removes the utf-8 character in front of the cursor position from the
 * line
 * @param line the line to modify
 * @param cursor_pos current position of the cursor
 * @return the number of bytes removed from line
 */
size_t line_delete(line_t *const line, const size_t cursor_pos);

/**
 * @brief copies the contents of the src line to the dest line, this delete the
 * contents of the dest line.
 * @param dest the line to copy to, the contents of this line are erased.
 * @param src the line to copy from
 */
void line_copy(line_t *dest, const line_t *const src);

#endif
