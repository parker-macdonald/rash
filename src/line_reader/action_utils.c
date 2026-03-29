#include "action_utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "jobs.h"
#include "lib/ansi.h"
#include "lib/error.h"
#include "lib/utf_8.h"
#include "line_reader/line_reader_struct.h"

void cursor_left(LineReader *reader) {
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

void cursor_right(LineReader *reader) {
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

void draw_active_buffer(LineReader *reader) {
  unsigned short width = get_terminal_width();

  unsigned moves_up = reader->cursor_pos / width;
  if (moves_up) {
    // move the cursor up `moves_up` times
    printf("\033[%uA", moves_up);
  }

  printf(
      "\r" ANSI_REMOVE_BELOW_CURSOR "%s%.*s",
      reader->prompt,
      (int)reader->active_buffer->length,
      reader->active_buffer->data
  );

  FLUSH();

  reader->buffer_offset = reader->active_buffer->length;
  reader->cursor_pos =
      reader->prompt_length + utf8_count_codepoint(reader->active_buffer);
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
  struct termios oldt = {0};
  struct termios newt = {0};
  uint8_t byte = 0;
  ssize_t nread;
  int saved_errno;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(unsigned int)(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  // this is to allow the read system call to know a sigint occured.
  dont_restart_on_sigint();

  for (;;) {
    errno = 0;
    nread = read(STDIN_FILENO, &byte, sizeof(byte));

    // adding a null terminator to the line buffer will fuck up the lexer
    if (nread == 1 && byte == '\0') {
      continue;
    }
    if (errno != EINTR) {
      saved_errno = errno;
      break;
    }
    if (recv_sigint == 1) {
      recv_sigint = 0;
      restart_on_sigint();
      tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings

      return SIGINT_ON_READ;
    }
  }

  restart_on_sigint();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings

  if (nread == 0) {
    return ASCII_END_OF_TRANSMISSION;
  }

  if (nread == -1) {
    error_f("read: %s\n", strerror(saved_errno));
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
