#include "line_reader/actions_all.h"

#include <stdio.h>
#include <string.h>

#include "lib/ansi.h"
#include "lib/utf_8.h"
#include "lib/buffer.h"
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

  printf(
      ANSI_CURSOR_HOME ANSI_ERASE_SCREEN "%s%.*s" ANSI_CURSOR_HOME,
      reader->prompt,
      (int)reader->active_buffer->length,
      reader->active_buffer->data
  );

  unsigned moves_down = reader->cursor_pos / width;
  if (moves_down) {
    // move the cursor down `moves_down` times
    printf("\033[%uB", moves_down);
  }

  printf("\033[%uC", reader->cursor_pos % width);

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
  return 0;
}

int action_cursor_right(LineReader *reader) {
  if (reader->buffer_offset >= reader->active_buffer->length) {
    return 0;
  }

  reader->buffer_offset =
      utf8_next_codepoint(reader->active_buffer, reader->buffer_offset);

  cursor_right(reader);
  return 0;
}

int action_end_of_file(LineReader *reader) {
  (void)reader;
  putchar('\n');
  return -1;
}

int action_sigint(LineReader *reader) {
  reader->history_curr = NULL;
  reader->active_buffer = &reader->buffer;
  reader->cursor_pos = reader->prompt_length;
  VECTOR_CLEAR(reader->buffer);

  printf("\n%s", reader->prompt);
  FLUSH();
  return 0;
}

int action_new_line(LineReader *reader) {
  if (reader->active_buffer->length == 0) {
    printf("\n%s", reader->prompt);
    FLUSH();
    return 0;
  }

  copy_hist_buf_if_needed(reader);

  putchar('\n');
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

  reader->buffer_offset = reader->history_curr->line.length;
  reader->cursor_pos =
      utf8_count_codepoint(&reader->history_curr->line) + reader->prompt_length;

  reader->active_buffer = &reader->history_curr->line;
  draw_active_buffer(reader);

  return 0;
}

int action_history_down(LineReader *reader) {
  if (reader->history_curr != NULL) {
    if (reader->history_curr->p_next != NULL) {
      reader->history_curr = reader->history_curr->p_next;
      reader->buffer_offset = reader->history_curr->line.length;
      reader->cursor_pos = utf8_count_codepoint(&reader->history_curr->line) +
                           reader->prompt_length;

      reader->active_buffer = &reader->history_curr->line;
      draw_active_buffer(reader);
      return 0;
    }

    reader->history_curr = NULL;
    reader->buffer_offset = reader->buffer.length;
    reader->cursor_pos =
        utf8_count_codepoint(&reader->buffer) + reader->prompt_length;

    reader->active_buffer = &reader->buffer;
    draw_active_buffer(reader);
    return 0;
  }

  return 0;
}

int action_insert(LineReader *reader, uint8_t byte) {
  copy_hist_buf_if_needed(reader);

  buffer_insert(&reader->buffer, reader->buffer_offset, byte);
  reader->buffer_offset++;

  if (!is_continuation_byte_utf8(byte)) {
    reader->cursor_pos++;

    if (reader->buffer_offset != reader->buffer.length) {
      printf(ANSI_INSERT_BLANK_CHAR "%c", byte);
      FLUSH();
      return 0;
    }
  }

  putchar(byte);

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

  // modified version of the cursor_left function
  {
    unsigned short width = get_terminal_width();

    if (reader->cursor_pos % width == 0) {
      // move cursor up one line and all the way to the right
      PUTS("\033[999C" ANSI_CURSOR_UP ANSI_DELETE_CHAR);
    } else {
      PUTS(ANSI_CURSOR_LEFT ANSI_DELETE_CHAR);
    }

    reader->cursor_pos--;

    FLUSH();
  }

  return 0;
}

int action_delete(LineReader *reader) {
  if (reader->active_buffer->length == reader->buffer_offset) {
    return 0;
  }

  copy_hist_buf_if_needed(reader);

  utf8_remove_codepoint(&reader->buffer, reader->buffer_offset);

  PUTS(ANSI_DELETE_CHAR);

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
  return 0;
}

int action_home(LineReader *reader) {
  unsigned short width = get_terminal_width();

  reader->buffer_offset = 0;

  unsigned moves_up = reader->cursor_pos / width;
  reader->cursor_pos = reader->prompt_length;

  if (moves_up > 0) {
    printf("\033[%uA", moves_up);
  }

  // move the cursort all the way to the left, then right prompt_length times
  printf("\r\033[%uC", reader->prompt_length);

  FLUSH();

  return 0;
}

int action_end(LineReader *reader) {
  unsigned short width = get_terminal_width();

  reader->buffer_offset = reader->active_buffer->length;

  unsigned length =
      utf8_count_codepoint(reader->active_buffer) + reader->prompt_length;

  // this is real code written by sane individuals
  unsigned moves_down = (length - reader->cursor_pos) / width;
  if (moves_down > 0) {
    printf("\033[%uB", moves_down);
  }

  reader->cursor_pos = length;
  printf("\r\033[%uC", length % width);

  FLUSH();

  return 0;
}

int action_complete(LineReader *reader) {
  auto_complete(reader);

  return 0;
}
