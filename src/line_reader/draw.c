#include "draw.h"

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "lib/ansi.h"
#include "lib/utf_8.h"
#include "line_reader/types.h"

void draw_entire_state(const LineReader *reader) {
  printf(
      ANSI_CURSOR_RESTORE ANSI_REMOVE_BELOW_CURSOR "%s%.*s ",
      reader->prompt,
      (int)reader->active_buffer->length,
      (char *)reader->active_buffer->u8_ptr
  );

  draw_cursor_at(reader, reader->cursor_pos);
}

void move_cursor_left(LineReader *reader) {
  unsigned short width = get_terminal_width();

  if (reader->cursor_pos % width == 0) {
    // move cursor up one line and all the way to the right
    PUTS(ANSI_CURSOR_RIGHT_N("999") ANSI_CURSOR_UP);
  } else {
    PUTS(ANSI_CURSOR_LEFT);
  }

  reader->cursor_pos--;
}

void move_cursor_right(LineReader *reader) {
  unsigned short width = get_terminal_width();

  if ((reader->cursor_pos + 1) % width == 0) {
    PUTS("\r\n");
  } else {
    PUTS(ANSI_CURSOR_RIGHT);
  }

  reader->cursor_pos++;
}

void move_cursor_left_n(LineReader *reader, unsigned n) {
  unsigned short width = get_terminal_width();

  unsigned moves_up =
      (reader->cursor_pos / width) - ((reader->cursor_pos - n) / width);

  if (moves_up > 0) {
    printf(ANSI_CURSOR_UP_N("%u"), moves_up);
  }

  unsigned x_pos = (reader->cursor_pos - n) % width;

  if (x_pos) {
    printf("\r" ANSI_CURSOR_RIGHT_N("%u"), x_pos);
  }

  reader->cursor_pos -= n;
}

void move_cursor_right_n(LineReader *reader, unsigned n) {
  unsigned short width = get_terminal_width();

  unsigned moves_down =
      ((reader->cursor_pos + n) / width) - (reader->cursor_pos / width);

  if (moves_down > 0) {
    printf(ANSI_CURSOR_DOWN_N("%u"), moves_down);
  }

  unsigned x_pos = (reader->cursor_pos + n) % width;

  if (x_pos) {
    printf("\r" ANSI_CURSOR_RIGHT_N("%u"), x_pos);
  }

  reader->cursor_pos += n;
}

void draw_cursor_post_line(const LineReader *reader) {
  unsigned short width = get_terminal_width();
  unsigned length = get_line_width(reader);
  unsigned current_line = reader->cursor_pos / width;
  unsigned total_lines = length / width;

  unsigned down = total_lines - current_line;
  if (down > 0) {
    printf(ANSI_CURSOR_DOWN_N("%u"), down);
  }

  PUTS("\r\n");
}

unsigned short get_terminal_width(void) {
  struct winsize win;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) != -1) {
    return win.ws_col;
  }
  // assume 80 columns if we cant get the terminal size
  return 80;
}

unsigned get_line_width(const LineReader *reader) {
  return reader->prompt_length + utf8_count_codepoint(reader->active_buffer);
}

void draw_cursor_at(const LineReader *reader, unsigned cursor_pos) {
  (void)reader;

  unsigned short width = get_terminal_width();

  unsigned down = cursor_pos / width;
  unsigned right = cursor_pos % width;

  PUTS(ANSI_CURSOR_RESTORE);

  if (down) {
    printf(ANSI_CURSOR_DOWN_N("%u"), down);
  }

  if (right) {
    printf(ANSI_CURSOR_RIGHT_N("%u"), right);
  }
}
