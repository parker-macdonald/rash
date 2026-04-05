#ifndef UTF_8_H
#define UTF_8_H

#include <stdbool.h>
#include <stddef.h>

#include "lib/buffer.h"

/**
 * @brief return the index of the first byte of the previous codepoint before
 * buffer_offset in buffer.
 * @param buffer the buffer to reader.
 * @param buffer_offset the offset into the buffer.
 * @return the index of the previous codepoint.
 */
size_t utf8_prev_codepoint(const Buffer *buffer, size_t buffer_offset);

size_t utf8_codepoint_size(const Buffer *buffer, size_t buffer_offset);

/**
 * @brief return the index of the first byte of the next codepoint after
 * buffer_offset in buffer.
 * @param buffer the buffer to read.
 * @param buffer_offset the offset into the buffer.
 * @return the index of the next codepoint.
 */
size_t utf8_next_codepoint(const Buffer *buffer, size_t buffer_offset);

/**
 * @brief Returns true when the given character is a continuation byte in utf-8.
 * @param byte The byte to check.
 * @return true: byte is a continuation byte, false: it is not.
 */
bool is_continuation_byte_utf8(uint8_t byte);

/**
 * @brief returns the number of codepoints in a buffer that is utf-8 encoded.
 * @param buffer the buffer to read.
 * @return the number of codepoints in the buffer.
 */
unsigned utf8_count_codepoint(const Buffer *buffer);

size_t utf8_remove_codepoint(Buffer *buffer, size_t buffer_offset);

#endif
