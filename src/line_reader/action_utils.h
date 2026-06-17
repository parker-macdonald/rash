#ifndef ACTION_UTILS_H
#define ACTION_UTILS_H

#include "lib/buffer.h"
#include "types.h"

// update the readers state (cursor pos, buffer offset, and active buffer
// pointer) with the new buffer (does not draw anything)
void update_active_buffer(LineReader *reader, Buffer *buffer);

void copy_hist_buf_if_needed(LineReader *reader);

size_t read_n_bytes(uint8_t *buf, size_t count);

uint8_t read_byte(void);

void next_word(const Buffer *buffer, size_t offset, unsigned *out_char_count, size_t *out_index);

void prev_word(const Buffer *buffer, size_t offset, unsigned *out_char_count, size_t *out_index);

#endif
