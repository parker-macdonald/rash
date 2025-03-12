#include "line_reader.h"
#include "ansi.h"
#include "utf_8.h"
#include "vector.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

char getch(void) {
  struct termios oldt;
  struct termios newt;
  char character;

  tcgetattr(STDIN_FILENO, &oldt);          // Get the current terminal settings
  newt = oldt;                             // Copy them to a new variable
  newt.c_lflag &= ~(ICANON | ECHO);        // Disable canonical mode and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &newt); // Set the new settings

  character = getchar(); // Read a single character

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings
  return character;
}

void line_insert(line_t *const line, const char read_char,
                 const unsigned int cursor_pos) {
  if (cursor_pos == line->length) {
    VECTOR_PUSH((*line), read_char);
    return;
  }

  char old = line->data[cursor_pos];
  line->data[cursor_pos] = read_char;

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

  const unsigned int offset = cursor_pos - char_size;
  line->length -= char_size;

  for (size_t i = offset; i < line->length; i++) {
    line->data[i] = line->data[i + char_size];
  }

  return char_size;
}

unsigned int delete(line_t *const line, const unsigned int cursor_pos) {
  const unsigned int char_size =
      traverse_forward_utf8(line->data, line->length, cursor_pos);

  if (line->length == cursor_pos + char_size) {
    line->length -= char_size;
    return char_size;
  }

  const unsigned int offset = cursor_pos;
  line->length -= char_size;

  for (size_t i = offset; i < line->length; i++) {
    line->data[i] = line->data[i + char_size];
  }

  return char_size;
}

void draw_line(const char *const prompt, const line_t *const line) {
  // save cursor pos since we're messing with the line
  fputs(ANSI_CURSOR_POS_SAVE, stdout);

  // the old line reader used to just redraw what changed, but that had lots of
  // bugs so now i'm just redrawing the whole line
  fputs(ANSI_REMOVE_FULL_LINE, stdout);
  // reset cursor to start of line
  fputs("\r", stdout);

  fputs(prompt, stdout);

  fwrite(line->data, sizeof(*line->data), line->length, stdout);

  // restore the cursor pos to where it was before
  fputs(ANSI_CURSOR_POS_RESTORE, stdout);
}

uint8_t *readline(uint8_t *data, const char *const prompt) {
  printf("%s", prompt);

  line_t line;

  line.capacity = VECTOR_DEFAULT_SIZE;
  line.length = 0;
  if (data == NULL) {
    line.data = malloc(sizeof(*line.data) * VECTOR_DEFAULT_SIZE);
  } else {
    line.data = data;
  }

  unsigned int cursor_pos = 0;

  char read_char;
  for (;;) {
    read_char = getch();

    if (read_char == ASCII_END_OF_TRANSMISSION) {
      printf("\n");
      VECTOR_DESTROY(line);
      return NULL;
    }

    if (read_char == '\n') {
      break;
    }

    // for when the user presses ctrl-c to trigger a sigint signal.
    if (read_char == EOF) {
      printf("^C");
      line.length = 0;
      break;
    }

    if (read_char == ANSI_START_CHAR) {
      read_char = getch();

      if (read_char != '[') {
        continue;
      }

      switch (getch()) {
      // right arrow
      case 'C':
        if (cursor_pos < line.length) {
          cursor_pos +=
              traverse_forward_utf8(line.data, line.length, cursor_pos);

          fputs(ANSI_CURSOR_RIGHT, stdout);
        }
        continue;
      // left arrow
      case 'D':
        if (cursor_pos > 0) {
          cursor_pos -= traverse_back_utf8(line.data, cursor_pos);

          fputs(ANSI_CURSOR_LEFT, stdout);
        }
        continue;
      case '1':
        if (getch() == ';') {
          if (getch() == '5') {
            const char arrow_char = getch();
            // shift right arrow
            if (arrow_char == 'C') {
              if (cursor_pos < line.length) {
                cursor_pos +=
                    traverse_forward_utf8(line.data, line.length, cursor_pos);
                fputs(ANSI_CURSOR_RIGHT, stdout);

                while (cursor_pos <= line.length - 1 &&
                       line.data[cursor_pos] != ' ') {
                  cursor_pos +=
                      traverse_forward_utf8(line.data, line.length, cursor_pos);
                  fputs(ANSI_CURSOR_RIGHT, stdout);
                }
              }

              continue;
            }
            // shift left arrow
            if (arrow_char == 'D') {
              if (cursor_pos > 0) {
                cursor_pos -= traverse_back_utf8(line.data, cursor_pos);
                fputs(ANSI_CURSOR_LEFT, stdout);

                while (cursor_pos > 0 && line.data[cursor_pos] != ' ') {
                  cursor_pos -= traverse_back_utf8(line.data, cursor_pos);
                  fputs(ANSI_CURSOR_LEFT, stdout);
                }
              }

              continue;
            }
          }
        }
      case '3':
        // delete key
        if (getch() == '~') {
          if (cursor_pos < line.length) {
            delete (&line, cursor_pos);
          }
          // should probably refactor to not use goto, but, i mean, it works...
          goto draw_line;
        }
        break;
      default:
        continue;
      }
    }

    // backspace
    if (read_char == ASCII_DEL) {
      if (cursor_pos > 0) {
        const unsigned int bytes_removed = backspace(&line, cursor_pos);
        cursor_pos -= bytes_removed;
        fputs(ANSI_CURSOR_LEFT, stdout);
      }

      goto draw_line;
    }

    line_insert(&line, read_char, cursor_pos);
    cursor_pos++;

    if (!is_continuation_byte_utf8(read_char)) {
      fputs(ANSI_CURSOR_RIGHT, stdout);
    }

  draw_line:

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
