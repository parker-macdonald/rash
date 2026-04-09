#include "line_reader/actions_all.h"

#include <stdio.h>
#include <string.h>

#include "lib/ansi.h"
#include "lib/buffer.h"
#include "lib/utf_8.h"
#include "line_reader/action_utils.h"
#include "line_reader/auto_complete.h"
#include "line_reader/history.h"
#include "line_reader/line_reader_struct.h"

int action_nop(LineReader *reader) {
  (void)reader;
  return 0;
}

int action_clear(LineReader *reader) {
  unsigned short width = get_terminal_width();

  PUTS(ANSI_CURSOR_HOME ANSI_ERASE_SCREEN);

  draw_active_buffer(reader);

  PUTS(ANSI_CURSOR_HOME);

  unsigned moves_down = reader->cursor_pos / width;
  if (moves_down) {
    printf(ANSI_CURSOR_DOWN_N("%u"), moves_down);
  }

  printf(ANSI_CURSOR_RIGHT_N("%u"), reader->cursor_pos % width);

  FLUSH();

  return 0;
}

int action_cursor_left(LineReader *reader) {
  if (reader->buffer_offset == 0) {
    return 0;
  }

  reader->buffer_offset =
      utf8_prev_codepoint(reader->active_buffer, reader->buffer_offset);

  cursor_left(reader);
  FLUSH();
  return 0;
}

int action_cursor_right(LineReader *reader) {
  if (reader->buffer_offset >= reader->active_buffer->length) {
    return 0;
  }

  reader->buffer_offset =
      utf8_next_codepoint(reader->active_buffer, reader->buffer_offset);

  cursor_right(reader);
  FLUSH();
  return 0;
}

int action_end_of_file(LineReader *reader) {
  cursor_to_bottom(reader);
  putchar('\n');
  return -1;
}

int action_sigint(LineReader *reader) {
  cursor_to_bottom(reader);
  printf("\n%s", reader->prompt);
  FLUSH();

  reader->history_curr = NULL;
  reader->active_buffer = &reader->buffer;

  reader->cursor_pos = reader->prompt_length;

  VECTOR_CLEAR(reader->buffer);

  return 0;
}

int action_new_line(LineReader *reader) {
  if (reader->active_buffer->length == 0) {
    printf("\n%s", reader->prompt);
    FLUSH();
    return 0;
  }

  cursor_to_bottom(reader);
  putchar('\n');

  copy_hist_buf_if_needed(reader);

  VECTOR_PUSH(reader->buffer, '\0');

  history_add(reader);

  return 1;
}

int action_history_up(LineReader *reader) {
  if (reader->history_curr == NULL && reader->history_end != NULL) {
    reader->history_curr = reader->history_end;
  } else if (reader->history_curr->p_prev != NULL) {
    reader->history_curr = reader->history_curr->p_prev;
  } else {
    // there is no more history
    return 0;
  }

  update_active_buffer(reader, &reader->history_curr->line);

  return 0;
}

int action_history_down(LineReader *reader) {
  if (reader->history_curr != NULL) {
    if (reader->history_curr->p_next != NULL) {
      reader->history_curr = reader->history_curr->p_next;

      update_active_buffer(reader, &reader->history_curr->line);
      return 0;
    }

    reader->history_curr = NULL;

    update_active_buffer(reader, &reader->buffer);
    return 0;
  }

  return 0;
}

int action_insert(LineReader *reader, uint8_t byte) {
  copy_hist_buf_if_needed(reader);

  buffer_insert(&reader->buffer, reader->buffer_offset, byte);
  reader->buffer_offset++;

  if (!is_continuation_byte_utf8(byte)) {
    cursor_right(reader);
  }

  PUTS(ANSI_CURSOR_POS_SAVE);

  draw_active_buffer(reader);

  PUTS(ANSI_CURSOR_POS_RESTORE);

  FLUSH();
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

  cursor_left(reader);

  PUTS(ANSI_CURSOR_POS_SAVE);

  draw_active_buffer(reader);

  PUTS(ANSI_CURSOR_POS_RESTORE);

  FLUSH();

  return 0;
}

int action_delete(LineReader *reader) {
  if (reader->active_buffer->length == reader->buffer_offset) {
    return 0;
  }

  copy_hist_buf_if_needed(reader);

  utf8_remove_codepoint(&reader->buffer, reader->buffer_offset);

  PUTS(ANSI_CURSOR_POS_SAVE);

  draw_active_buffer(reader);

  PUTS(ANSI_CURSOR_POS_RESTORE);

  FLUSH();

  return 0;
}

int action_word_left(LineReader *reader) {
  if (reader->buffer_offset == 0) {
    return 0;
  }

  unsigned count = 1;

  reader->buffer_offset =
      utf8_prev_codepoint(reader->active_buffer, reader->buffer_offset);

  while (reader->buffer_offset > 0 &&
         reader->active_buffer->data[reader->buffer_offset - 1] != ' ') {
    reader->buffer_offset =
        utf8_prev_codepoint(reader->active_buffer, reader->buffer_offset);
    count++;
  }

  cursor_left_n(reader, count);
  FLUSH();
  return 0;
}

int action_word_right(LineReader *reader) {
  if (reader->active_buffer->length == reader->buffer_offset) {
    return 0;
  }

  unsigned count = 1;

  reader->buffer_offset =
      utf8_next_codepoint(reader->active_buffer, reader->buffer_offset);

  while (reader->buffer_offset <= reader->active_buffer->length - 1 &&
         reader->active_buffer->data[reader->buffer_offset] != ' ') {
    reader->buffer_offset =
        utf8_next_codepoint(reader->active_buffer, reader->buffer_offset);
    count++;
  }

  cursor_right_n(reader, count);
  FLUSH();
  return 0;
}

int action_home(LineReader *reader) {
  unsigned short width = get_terminal_width();

  reader->buffer_offset = 0;

  unsigned moves_up = reader->cursor_pos / width;
  reader->cursor_pos = reader->prompt_length;

  if (moves_up > 0) {
    printf(ANSI_CURSOR_UP_N("%u"), moves_up);
  }

  // move the cursort all the way to the left, then right prompt_length times
  printf("\r" ANSI_CURSOR_RIGHT_N("%u"), reader->prompt_length);

  FLUSH();

  return 0;
}

int action_end(LineReader *reader) {
  unsigned short width = get_terminal_width();

  unsigned length = get_line_length(reader);

  reader->buffer_offset = reader->active_buffer->length;

  unsigned current_line = reader->cursor_pos / width;
  unsigned total_lines = length / width;

  unsigned down = total_lines - current_line;
  if (down > 0) {
    printf(ANSI_CURSOR_DOWN_N("%u"), down);
  }

  reader->cursor_pos = length;

  putchar('\r');

  unsigned right = length % width;
  if (right) {
    printf(ANSI_CURSOR_RIGHT_N("%u"), right);
  }

  FLUSH();

  return 0;
}

int action_complete(LineReader *reader) {
  auto_complete(reader);

  return 0;
}
