#include "line_reader.h"
#include "ansi.h"
#include "utf_8.h"
#include <assert.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

char getch(void) {
  struct termios oldt, newt;
  char ch;

  tcgetattr(STDIN_FILENO, &oldt);          // Get the current terminal settings
  newt = oldt;                             // Copy them to a new variable
  newt.c_lflag &= ~(ICANON | ECHO);        // Disable canonical mode and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &newt); // Set the new settings

  ch = getchar(); // Read a single character

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings
  return ch;
}

void line_insert(line_t *const line, const char c,
                 const unsigned int cursor_pos) {
  if (cursor_pos == line->length) {
    VECTOR_PUSH((*line), c);
    return;
  }

  char old = line->data[cursor_pos];
  line->data[cursor_pos] = c;

  for (size_t i = cursor_pos + 1; i < line->length; i++) {
    char old2 = line->data[i];

    line->data[i] = old;

    old = old2;
  }

  VECTOR_PUSH((*line), old);
}

/**
 * @brief deletes the utf-8 character before cursor position
 * @param line the line to remove data from
 * @param cursor_pos the current position of the cursor, this function deletes
 * the character before the cursor position
 * @return the number of bytes removed
 */
unsigned int backspace(line_t *const line, const unsigned int cursor_pos) {
  const unsigned int char_size = traverse_back_utf8(line->data, cursor_pos);

  if (line->length == cursor_pos) {
    line->length -= char_size;
    return char_size;
  }

  const unsigned int offset = cursor_pos - char_size - 1;
  line->length -= char_size;

  for (size_t i = offset; i < line->length; i++) {
    line->data[i] = line->data[i + char_size];
  }

  return char_size;
}

unsigned int get_utf8_char_len_forward(const char c) {
  if ((c & 0xe0) == 0xc0) {
    return 2;
  }

  if ((c & 0xf0) == 0xe0) {
    return 3;
  }

  if ((c & 0xf8) == 0xf0) {
    return 4;
  }

  return 1;
}

char *readline(char *data, const char *const prompt) {
  printf("%s", prompt);

  line_t line;

  line.capacity = 16;
  line.length = 0;
  if (data == NULL) {
    line.data = malloc(sizeof(*line.data) * 16);
  } else {
    line.data = data;
  }

  unsigned int cursor_pos = 0;

  char c;
  for (;;) {
    c = getch();

    if (c == ASCII_END_OF_TRANSMISSION) {
      printf("\n");
      return NULL;
    }

    if (c == '\n') {
      break;
    }

    if (c == ANSI_START_CHAR) {
      c = getch();

      if (c != '[') {
        continue;
      }

      switch (getch()) {
      // right arrow
      case 'C':
        if (cursor_pos < line.length) {
          cursor_pos += get_utf8_char_len_forward(c);

          fputs(ANSI_CURSOR_RIGHT, stdout);
        }
        break;
      // left arrow
      case 'D':
        if (cursor_pos > 0) {
          cursor_pos -= traverse_back_utf8(line.data, cursor_pos);

          fputs(ANSI_CURSOR_LEFT, stdout);
        }
        break;
      }

      continue;
      // backspace
    } else if (c == 0x7f) {
      if (cursor_pos > 0) {
        const unsigned int bytes_removed = backspace(&line, cursor_pos);
        cursor_pos -= bytes_removed;
        fputs(ANSI_CURSOR_LEFT, stdout);
      }
    } else {
      line_insert(&line, c, cursor_pos);
      cursor_pos++;

      if (!(c & 0x80) || !(~c & 0xc0)) {
        fputs(ANSI_CURSOR_RIGHT, stdout);
      }
    }

    fputs(ANSI_CURSOR_POS_SAVE, stdout);

    fputs(ANSI_REMOVE_FULL_LINE, stdout);
    // reset cursor to start of line
    printf("\r");

    printf("%s", prompt);
    for (size_t i = 0; i < line.length; i++) {
      printf("%c", line.data[i]);
    }

    fputs(ANSI_CURSOR_POS_RESTORE, stdout);
  }

  VECTOR_PUSH(line, '\0');

  printf("\n");

  return line.data;
}
