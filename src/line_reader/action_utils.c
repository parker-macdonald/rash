#include "action_utils.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "lib/ansi.h"
#include "lib/error.h"
#include "lib/utf_8.h"
#include "line_reader/actions.h"
#include "line_reader/line_reader_struct.h"

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

int getch(void) {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);

  sigset_t set;
  sigemptyset(&set);

  if (pselect(1, &fds, NULL, NULL, NULL, &set) == -1) {
    if (errno == EINTR) {
      return SIGINT_ON_READ;
    }

    fatal_f("pselect: %s\n", strerror(errno));
  }

  uint8_t byte = 0;
  ssize_t nread = read(STDIN_FILENO, &byte, sizeof(byte));

  if (nread == 0) {
    return ASCII_END_OF_TRANSMISSION;
  }

  if (nread == -1) {
    error_f("read: %s\n", strerror(errno));
    return ASCII_END_OF_TRANSMISSION;
  }

  return (int)byte;
}

void copy_hist_buf_if_needed(LineReader *reader) {
  if (reader->history_curr != NULL) {
    buffer_copy(&reader->buffer, &reader->history_curr->line);
    reader->active_buffer = &reader->buffer;
    reader->history_curr = NULL;
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
