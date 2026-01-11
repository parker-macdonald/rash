#ifndef MODIFY_LINE_H
#define MODIFY_LINE_H

#include "lib/vec_types.h"

/**
 * @brief adds the specified byte to the line at the cursor position
 * @param line the line to modify
 * @param byte the byte to add to the line
 * @param cursor_pos where the byte should go in the line
 */
void line_insert(buf_t *line, uint8_t byte, size_t cursor_pos);

/**
 * @brief removes the utf-8 character behind the cursor position from the line
 * @param line the line to modify
 * @param cursor_pos current position of the cursor
 * @return the number of bytes removed from line
 */
size_t line_backspace(buf_t *line, size_t cursor_pos);

/**
 * @brief removes the utf-8 character in front of the cursor position from the
 * line
 * @param line the line to modify
 * @param cursor_pos current position of the cursor
 * @return the number of bytes removed from line
 */
size_t line_delete(buf_t *line, size_t cursor_pos);

/**
 * @brief copies the contents of the src line to the dest line, this delete the
 * contents of the dest line.
 * @param dest the line to copy to, the contents of this line are erased.
 * @param src the line to copy from
 */
void line_copy(buf_t *dest, const buf_t *src);

/**
 * @brief insert src_len bytes from src into line at index cursor_pos
 * @param line the line to modify
 * @param cursor_pos where to insert data into line, line->data[cursor_pos]
 * contains the first new character
 * @param src buffer containing the bytes to write to line
 * @param src_len the amount of bytes to read from src
 */
void line_insert_bulk(
    buf_t *line, size_t cursor_pos, const uint8_t *src, size_t src_len
);

#endif
