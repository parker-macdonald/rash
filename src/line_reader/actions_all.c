#include "line_reader/actions_all.h"

#include <stdint.h>
#include <stdio.h>

#include "lib/ansi.h"
#include "lib/buffer.h"
#include "lib/utf_8.h"
#include "lib/vector.h"
#include "line_reader/action_utils.h"
#include "line_reader/auto_complete.h"
#include "line_reader/draw.h"
#include "line_reader/history.h"
#include "line_reader/types.h"

int action_nop(LineReader *reader) {
  (void)reader;
  return 0;
}

int action_clear(LineReader *reader) {
  PUTS(ANSI_CURSOR_HOME ANSI_ERASE_SCREEN ANSI_CURSOR_SAVE);

  draw_entire_state(reader);
  draw_flush();

  return 0;
}

int action_cursor_left(LineReader *reader) {
  if (reader->buffer_offset == 0) {
    return 0;
  }

  reader->buffer_offset =
      utf8_prev_codepoint(reader->active_buffer, reader->buffer_offset);

  move_cursor_left(reader);
  draw_flush();

  return 0;
}

int action_cursor_right(LineReader *reader) {
  if (reader->buffer_offset >= reader->active_buffer->length) {
    return 0;
  }

  reader->buffer_offset =
      utf8_next_codepoint(reader->active_buffer, reader->buffer_offset);

  move_cursor_right(reader);
  draw_flush();

  return 0;
}

int action_stop(LineReader *reader) {
  draw_cursor_post_line(reader);

  return -1;
}

int action_clear_line(LineReader *reader) {
  reader->history_curr = reader->history.length;

  reader->active_buffer = &reader->buffer;
  VECTOR_CLEAR(reader->buffer);

  reader->cursor_pos = reader->prompt_length;
  reader->buffer_offset = 0;

  draw_cursor_post_line(reader);
  PUTS(ANSI_CURSOR_SAVE);
  draw_entire_state(reader);
  draw_flush();

  return 0;
}

int action_new_line(LineReader *reader) {
  if (reader->active_buffer->length == 0) {
    printf("\n%s", reader->prompt);
    draw_flush();
    return 0;
  }

  copy_hist_buf_if_needed(reader);

  history_add(reader);

  draw_cursor_post_line(reader);
  draw_flush();

  return 1;
}

int action_history_up(LineReader *reader) {
  if (reader->history_curr == 0) {
    return 0;
  }

  reader->history_curr--;

  update_active_buffer(reader, &reader->history.data[reader->history_curr]);
  draw_entire_state(reader);
  draw_flush();

  return 0;
}

int action_history_down(LineReader *reader) {
  if (reader->history_curr == reader->history.length) {
    return 0;
  }

  reader->history_curr++;

  if (reader->history_curr == reader->history.length) {
    update_active_buffer(reader, &reader->buffer);
    draw_entire_state(reader);
    draw_flush();
    return 0;
  }

  update_active_buffer(reader, &reader->history.data[reader->history_curr]);
  draw_entire_state(reader);
  draw_flush();

  return 0;
}

int action_insert(LineReader *reader, uint8_t byte) {
  copy_hist_buf_if_needed(reader);

  buffer_insert_byte(&reader->buffer, reader->buffer_offset, byte);
  reader->buffer_offset++;

  if (!is_continuation_byte_utf8(byte)) {
    reader->cursor_pos++;
  }

  draw_entire_state(reader);
  draw_flush();

  return 0;
}

int action_backspace(LineReader *reader) {
  if (reader->buffer_offset == 0) {
    return 0;
  }

  copy_hist_buf_if_needed(reader);

  size_t previous = utf8_prev_codepoint(&reader->buffer, reader->buffer_offset);
  utf8_remove_codepoint(&reader->buffer, previous);
  reader->buffer_offset = previous;
  reader->cursor_pos--;

  draw_entire_state(reader);
  draw_flush();

  return 0;
}

int action_delete(LineReader *reader) {
  if (reader->active_buffer->length == reader->buffer_offset) {
    return 0;
  }

  copy_hist_buf_if_needed(reader);

  utf8_remove_codepoint(&reader->buffer, reader->buffer_offset);

  draw_entire_state(reader);
  draw_flush();

  return 0;
}

int action_word_left(LineReader *reader) {
  if (reader->buffer_offset == 0) {
    return 0;
  }

  unsigned char_count;
  size_t index;
  prev_word(reader->active_buffer, reader->buffer_offset, &char_count, &index);

  reader->buffer_offset = index;

  move_cursor_left_n(reader, char_count);
  draw_flush();
  return 0;
}

int action_word_right(LineReader *reader) {
  if (reader->active_buffer->length == reader->buffer_offset) {
    return 0;
  }

  unsigned char_count;
  size_t index;
  next_word(reader->active_buffer, reader->buffer_offset, &char_count, &index);

  reader->buffer_offset = index;

  move_cursor_right_n(reader, char_count);
  draw_flush();
  return 0;
}

int action_home(LineReader *reader) {
  reader->buffer_offset = 0;
  reader->cursor_pos = reader->prompt_length;

  draw_cursor_at(reader, reader->prompt_length);
  draw_flush();

  return 0;
}

int action_end(LineReader *reader) {
  unsigned length = get_line_width(reader);

  reader->buffer_offset = reader->active_buffer->length;
  reader->cursor_pos = length;

  draw_cursor_at(reader, length);
  draw_flush();

  return 0;
}

int action_complete(LineReader *reader) {
  auto_complete(reader);

  return 0;
}

int action_delete_word_left(LineReader *reader) {
  if (reader->buffer_offset == 0) {
    return 0;
  }

  unsigned char_count;
  size_t index;
  prev_word(reader->active_buffer, reader->buffer_offset, &char_count, &index);

  copy_hist_buf_if_needed(reader);

  buffer_remove_n(reader->active_buffer, index, reader->buffer_offset - index);

  reader->buffer_offset = index;
  reader->cursor_pos -= char_count;

  draw_entire_state(reader);
  draw_flush();

  return 0;
}

int action_delete_word_right(LineReader *reader) {
  if (reader->active_buffer->length == reader->buffer_offset) {
    return 0;
  }

  unsigned char_count;
  size_t index;
  prev_word(reader->active_buffer, reader->buffer_offset, &char_count, &index);

  copy_hist_buf_if_needed(reader);

  buffer_remove_n(
      reader->active_buffer,
      reader->buffer_offset,
      index - reader->buffer_offset
  );

  draw_entire_state(reader);
  draw_flush();

  return 0;
}
