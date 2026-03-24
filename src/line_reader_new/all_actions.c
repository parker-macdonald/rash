#include <stdio.h>

#include "lib/ansi.h"
#include "lib/utf_8.h"
#include "lib/vec_types.h"
#include "line_reader/modify_line.h"
#include "line_reader/utils.h"
#include "line_reader_new/history.h"
#include "line_reader_new/line_reader.h"
#include "line_reader_new/line_reader_struct.h"

#define PUTS(str) (void)fputs(str, stdout)
#define FLUSH() (void)fflush(stdout)

static void cursor_left(line_reader *reader) {
  unsigned short width = get_terminal_width();

  if (reader->cursor_pos % width == 0) {
    // move cursor up one line and all the way to the right
    PUTS("\033[999C\033[A");
  } else {
    PUTS(ANSI_CURSOR_LEFT);
  }

  reader->cursor_pos--;

  FLUSH();
}

static void cursor_right(line_reader *reader) {
  unsigned short width = get_terminal_width();

  reader->cursor_pos++;
  if (reader->cursor_pos % width == 0) {
    // move cursor up one line and all the way to the right
    PUTS("\r" ANSI_CURSOR_DOWN);
  } else {
    PUTS(ANSI_CURSOR_RIGHT);
  }

  FLUSH();
}

static void draw_buffer(line_reader *reader, const buf_t *buffer) {
  unsigned short width = get_terminal_width();

  unsigned moves_up = reader->cursor_pos / width;
  if (moves_up) {
    // move the cursor up `moves_up` times
    printf("\033[%uA", moves_up);
  }

  printf(
      "\r" ANSI_REMOVE_BELOW_CURSOR "%s%.*s",
      reader->prompt,
      (int)buffer->length,
      buffer->data
  );

  FLUSH();

  reader->buffer_offset = buffer->length;
  reader->cursor_pos =
      reader->prompt_length + strlen_utf8(buffer->data, buffer->length);
}

int action_nop(line_reader *reader) {
  (void)reader;
  return 0;
}

int action_clear(line_reader *reader) {
  unsigned short width = get_terminal_width();

  printf(
      ANSI_CURSOR_HOME ANSI_ERASE_SCREEN "%s%.*s" ANSI_CURSOR_HOME,
      reader->prompt,
      (int)reader->buffer.length,
      reader->buffer.data
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

int action_cursor_left(line_reader *reader) {
  if (reader->buffer_offset == 0) {
    return 0;
  }

  reader->buffer_offset -=
      traverse_back_utf8(reader->buffer.data, reader->buffer_offset);

  cursor_left(reader);
  return 0;
}

int action_cursor_right(line_reader *reader) {
  if (reader->buffer_offset >= reader->buffer.length) {
    return 0;
  }

  reader->buffer_offset += traverse_forward_utf8(
      reader->buffer.data, reader->buffer.length, reader->buffer_offset
  );

  cursor_right(reader);
  return 0;
}

int action_end_of_file(line_reader *reader) {
  (void)reader;
  putchar('\n');
  return -1;
}

int action_sigint(line_reader *reader) {
  reader->history_curr = NULL;
  reader->cursor_pos = reader->prompt_length;
  VECTOR_CLEAR(reader->buffer);

  printf("\n%s", reader->prompt);
  FLUSH();
  return 0;
}

int action_new_line(line_reader *reader) {
  if (reader->history_curr != NULL) {
    buf_copy(&reader->buffer, &reader->history_curr->line);
    reader->history_curr = NULL;
  }

  if (reader->buffer.length == 0) {
    printf("\n%s", reader->prompt);
    FLUSH();
    return 0;
  }

  putchar('\n');
  VECTOR_PUSH(reader->buffer, '\0');

  history_add(reader);

  return 1;
}

int action_history_up(line_reader *reader) {
  // if there's no current line, assign it to the last node
  if (reader->history_curr == NULL) {
    if (reader->history_end != NULL) {
      reader->history_curr = reader->history_end;
    } else {
      return 0;
    }
    // if there is a current line, assign it to the previous one
  } else {
    if (reader->history_curr->p_prev != NULL) {
      reader->history_curr = reader->history_curr->p_prev;
    } else {
      return 0;
    }
  }

  draw_buffer(reader, &reader->history_curr->line);
  return 0;
}

int action_insert(line_reader *reader, uint8_t byte) {
  if (reader->history_curr != NULL) {
    buf_copy(&reader->buffer, &reader->history_curr->line);
    reader->history_curr = NULL;
  }

  line_insert(&reader->buffer, byte, reader->buffer_offset);
  reader->buffer_offset++;

  if (is_continuation_byte_utf8(byte)) {
    return 0;
  }

  reader->cursor_pos++;

  if (reader->buffer_offset == reader->buffer.length) {
    putchar(byte);
    FLUSH();
    return 0;
  }

  printf(ANSI_INSERT_BLANK_CHAR "%c", byte);
  FLUSH();
  return 0;
}

int action_backspace(line_reader *reader) {
  if (reader->history_curr != NULL) {
    buf_copy(&reader->buffer, &reader->history_curr->line);
    reader->history_curr = NULL;
  }
  
  if (reader->buffer_offset == 0) {
    return 0;
  }

  const size_t bytes_removed =
      line_backspace(&reader->buffer, reader->buffer_offset);
  reader->buffer_offset -= bytes_removed;

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
