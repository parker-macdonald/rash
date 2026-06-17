#include "action_utils.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/utf_8.h"
#include "line_reader/draw.h"
#include "line_reader/history.h"
#include "line_reader/types.h"

size_t read_n_bytes(uint8_t *buf, size_t count) {
  ssize_t nread = read(STDIN_FILENO, buf, count);

  if (nread == 0) {
    fatal("\nread: no characters were read, something crazy happened.\n");
  }

  if (nread == -1) {
    fatal_f("\nread: %s\n", strerror(errno));
  }

  return (size_t)nread;
}

uint8_t read_byte(void) {
  uint8_t byte = 0;

  read_n_bytes(&byte, 1);

  return byte;
}

void copy_hist_buf_if_needed(LineReader *reader) {
  if (reader->history_curr != reader->history.length) {
    buffer_copy(&reader->buffer, history_curr(reader));
    reader->active_buffer = &reader->buffer;
    reader->history_curr = reader->history.length;
  }
}

void update_active_buffer(LineReader *reader, Buffer *buffer) {
  reader->active_buffer = buffer;
  reader->buffer_offset = buffer->length;

  reader->cursor_pos = get_line_width(reader);
}

void next_word(
    const Buffer *buffer,
    size_t offset,
    unsigned *out_char_count,
    size_t *out_index
) {
  if (buffer->length == offset) {
    *out_char_count = 0;
    *out_index = offset;
    return;
  }

  unsigned count = 1;

  offset = utf8_next_codepoint(buffer, offset);

  while (offset <= buffer->length - 1 && buffer->data[offset] != ' ') {
    offset = utf8_next_codepoint(buffer, offset);
    count++;
  }

  *out_index = offset;
  *out_char_count = count;
}

void prev_word(
    const Buffer *buffer,
    size_t offset,
    unsigned *out_char_count,
    size_t *out_index
) {
  if (offset == 0) {
    *out_index = 0;
    *out_char_count = 0;
    return;
  }

  unsigned count = 1;

  offset = utf8_prev_codepoint(buffer, offset);

  while (offset > 0 && buffer->data[offset - 1] != ' ') {
    offset = utf8_prev_codepoint(buffer, offset);
    count++;
  }

  *out_index = offset;
  *out_char_count = count;
}
