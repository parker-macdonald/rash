#include "action_utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "lib/ansi.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/utf_8.h"
#include "line_reader/history.h"
#include "line_reader/types.h"

void cursor_left(LineReader *reader) {
  unsigned short width = get_terminal_width();

  if (reader->cursor_pos % width == 0) {
    // move cursor up one line and all the way to the right
    PUTS(ANSI_CURSOR_RIGHT_N("999") ANSI_CURSOR_UP);
  } else {
    PUTS(ANSI_CURSOR_LEFT);
  }

  reader->cursor_pos--;
}

void cursor_right(LineReader *reader) {
  unsigned short width = get_terminal_width();

  reader->cursor_pos++;
  if (reader->cursor_pos % width == 0) {
    PUTS("\r\n");
  } else {
    PUTS(ANSI_CURSOR_RIGHT);
  }
}

void cursor_left_n(LineReader *reader, unsigned n) {
  unsigned short width = get_terminal_width();

  unsigned moves_up =
      (reader->cursor_pos / width) - ((reader->cursor_pos - n) / width);

  if (moves_up > 0) {
    printf(ANSI_CURSOR_UP_N("%u"), moves_up);
  }

  reader->cursor_pos -= n;
  unsigned x_pos = reader->cursor_pos % width;

  if (x_pos) {
    printf("\r" ANSI_CURSOR_RIGHT_N("%u"), x_pos);
  }
}

void cursor_right_n(LineReader *reader, unsigned n) {
  unsigned short width = get_terminal_width();

  unsigned moves_down =
      ((reader->cursor_pos + n) / width) - (reader->cursor_pos / width);

  if (moves_down > 0) {
    printf(ANSI_CURSOR_DOWN_N("%u"), moves_down);
  }

  reader->cursor_pos += n;
  unsigned x_pos = reader->cursor_pos % width;

  if (x_pos) {
    printf("\r" ANSI_CURSOR_RIGHT_N("%u"), x_pos);
  }
}

void draw_active_buffer(LineReader *reader) {
  unsigned short width = get_terminal_width();

  unsigned moves_up = reader->cursor_pos / width;
  if (moves_up > 0) {
    printf(ANSI_CURSOR_UP_N("%u"), moves_up);
  }

  printf(
      "\r\033[0J%s%.*s ",
      reader->prompt,
      (int)reader->active_buffer->length,
      (char *)reader->active_buffer->data
  );
}

void update_active_buffer(LineReader *reader, Buffer *buffer) {
  reader->active_buffer = buffer;
  reader->buffer_offset = buffer->length;

  draw_active_buffer(reader);

  PUTS(ANSI_CURSOR_LEFT);

  FLUSH();

  reader->cursor_pos = get_line_length(reader);
}

unsigned short get_terminal_width(void) {
  struct winsize win;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) != -1) {
    return win.ws_col;
  }
  // assume 80 columns if we cant get the terminal size
  return 80;
}

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

unsigned get_line_length(const LineReader *reader) {
  return reader->prompt_length + utf8_count_codepoint(reader->active_buffer);
}

void cursor_to_bottom(const LineReader *reader) {
  unsigned short width = get_terminal_width();
  unsigned length = get_line_length(reader);
  unsigned current_line = reader->cursor_pos / width;
  unsigned total_lines = length / width;

  unsigned down = total_lines - current_line;
  if (down > 0) {
    printf(ANSI_CURSOR_DOWN_N("%u"), down);
  }

  putchar('\r');
}
