#include "line_reader.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <termios.h>
#include <unistd.h>

#include "ansi.h"
#include "utf_8.h"
#include "vector.h"

_Static_assert(sizeof(char) == sizeof(uint8_t),
               "char is not one byte in size, god save you...");

line_node_t *root_line_node = NULL;
static line_node_t *last_line_node = NULL;

void line_reader_destroy(void) {
  line_node_t *node = last_line_node;

  while (node != NULL) {
    VECTOR_DESTROY(node->line);

    line_node_t *prev_node = node->p_prev;

    free(node);
    node = NULL;

    node = prev_node;
  }

  root_line_node = NULL;
  last_line_node = NULL;
}

static uint8_t getch(void) {
  struct termios oldt;
  struct termios newt;
  uint8_t byte;

  tcgetattr(STDIN_FILENO, &oldt); // Get the current terminal settings
  newt = oldt;                    // Copy them to a new variable
  newt.c_lflag &=
      ~(unsigned int)(ICANON | ECHO);      // Disable canonical mode and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &newt); // Set the new settings

  size_t n_read =
      fread(&byte, sizeof(byte), 1, stdin); // Read a single character

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings

  if (n_read != 1) {
    return EOF;
  }

  return byte;
}

static void line_insert(line_t *const line, const uint8_t byte,
                        const size_t cursor_pos) {
  if (cursor_pos == line->length) {
    VECTOR_PUSH((*line), byte);
    return;
  }

  uint8_t old = line->data[cursor_pos];
  line->data[cursor_pos] = byte;

  for (size_t i = cursor_pos + 1; i < line->length; i++) {
    const uint8_t old2 = line->data[i];

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
static size_t backspace(line_t *const line, const size_t cursor_pos) {
  const size_t char_size = traverse_back_utf8(line->data, cursor_pos);

  if (line->length == cursor_pos) {
    line->length -= char_size;
    return char_size;
  }

  const size_t offset = cursor_pos - char_size;
  line->length -= char_size;

  for (size_t i = offset; i < line->length; i++) {
    line->data[i] = line->data[i + char_size];
  }

  return char_size;
}

static size_t delete(line_t *const line, const size_t cursor_pos) {
  const size_t char_size =
      traverse_forward_utf8(line->data, line->length, cursor_pos);

  if (line->length == cursor_pos + char_size) {
    line->length -= char_size;
    return char_size;
  }

  const size_t offset = cursor_pos;
  line->length -= char_size;

  for (size_t i = offset; i < line->length; i++) {
    line->data[i] = line->data[i + char_size];
  }

  return char_size;
}

static void draw_line(const char *const prompt, const line_t *const line) {
  // the old line reader used to just redraw what changed, but that had lots of
  // bugs so now i'm just redrawing the whole line
  fputs(ANSI_REMOVE_FULL_LINE, stdout);
  // reset cursor to start of line
  fputs("\r", stdout);

  fputs(prompt, stdout);

  PRINT_LINE(*line);
}

void clear_history(void) {
  line_node_t *node = root_line_node;

  while (node != NULL) {
    line_node_t *next_node = node->p_next;

    free(node);

    node = next_node;
  }
  root_line_node = NULL;
  last_line_node = NULL;
}

static void line_copy(const line_t *const src, line_t *dest) {
  VECTOR_CLEAR(*dest);

  for (size_t i = 0; i < src->length; i++) {
    VECTOR_PUSH(*dest, src->data[i]);
  }
}

const uint8_t *readline(const char *const prompt) {
  printf("%s", prompt);

  line_node_t *node = NULL;

  line_t line;
  VECTOR_INIT(line);

  size_t cursor_pos = 0;

  uint8_t curr_byte;
  for (;;) {
    curr_byte = getch();

    if (curr_byte == ASCII_END_OF_TRANSMISSION) {
      printf("\n");
      VECTOR_DESTROY(line);
      line_reader_destroy();
      return NULL;
    }

    // this happens when the user presses ctrl-c, ctrl-z, sometimes when a
    // child process exits, not sure why, but just ignore the byte and move
    // to a new line.
    if ((uint8_t)curr_byte == RECV_SIGINT) {
      printf("\n%s", prompt);
      line.length = 0;
      continue;
    }

    // readonly line to get length or data info from. when the user is going
    // through history, node contains the line they are viewing whereas line
    // contains the buffer the user is currently editting
    line_t *line_to_read = node == NULL ? &line : &node->line;

    if (curr_byte == '\n') {
      if (line_to_read->length != 0) {
        line_node_t *new_node = malloc(sizeof(line_node_t));
        if (node != NULL) {
          line_copy(&node->line, &line);
        }

        new_node->line = line;

        new_node->p_next = NULL;
        new_node->p_prev = last_line_node;
        if (last_line_node != NULL) {
          last_line_node->p_next = new_node;
        } else {
          root_line_node = new_node;
        }
        last_line_node = new_node;
        break;
      }

      printf("\n%s", prompt);
      continue;
    }

    if (curr_byte == ANSI_START_CHAR) {
      curr_byte = getch();

      if (curr_byte != '[') {
        continue;
      }

      switch (getch()) {
      // arrow up
      case 'A':
        if (node == NULL) {
          if (last_line_node != NULL) {
            node = last_line_node;
            cursor_pos = node->line.length;
            draw_line(prompt, &node->line);
          }
        } else if (node->p_prev != NULL) {
          node = node->p_prev;
          cursor_pos = node->line.length;
          draw_line(prompt, &node->line);
        }
        continue;
      // arrow down
      case 'B':
        if (node != NULL && node->p_next != NULL) {
          node = node->p_next;
          cursor_pos = node->line.length;
          draw_line(prompt, &node->line);
        } else {
          node = NULL;
          cursor_pos = line.length;
          draw_line(prompt, &line);
        }
        continue;
      // right arrow
      case 'C':
        if (cursor_pos < line_to_read->length) {
          cursor_pos += traverse_forward_utf8(line_to_read->data,
                                              line_to_read->length, cursor_pos);

          fputs(ANSI_CURSOR_RIGHT, stdout);
        }
        continue;
      // left arrow
      case 'D':
        if (cursor_pos > 0) {
          cursor_pos -= traverse_back_utf8(line_to_read->data, cursor_pos);

          fputs(ANSI_CURSOR_LEFT, stdout);
        }
        continue;
      case '1':
        if (getch() == ';') {
          if (getch() == '5') {
            const uint8_t arrow_char = getch();
            // shift right arrow
            if (arrow_char == 'C') {
              if (cursor_pos < line_to_read->length) {
                cursor_pos += traverse_forward_utf8(
                    line_to_read->data, line_to_read->length, cursor_pos);
                fputs(ANSI_CURSOR_RIGHT, stdout);

                while (cursor_pos <= line_to_read->length - 1 &&
                       line_to_read->data[cursor_pos] != ' ') {
                  cursor_pos += traverse_forward_utf8(
                      line_to_read->data, line_to_read->length, cursor_pos);
                  fputs(ANSI_CURSOR_RIGHT, stdout);
                }
              }

              continue;
            }
            // shift left arrow
            if (arrow_char == 'D') {
              if (cursor_pos > 0) {
                cursor_pos -=
                    traverse_back_utf8(line_to_read->data, cursor_pos);
                fputs(ANSI_CURSOR_LEFT, stdout);

                while (cursor_pos > 0 &&
                       line_to_read->data[cursor_pos] != ' ') {
                  cursor_pos -=
                      traverse_back_utf8(line_to_read->data, cursor_pos);
                  fputs(ANSI_CURSOR_LEFT, stdout);
                }
              }

              continue;
            }
          }
        }
        break;
      case '3':
        // delete key
        if (getch() == '~') {
          if (cursor_pos < line_to_read->length) {
            if (node != NULL) {
              line_copy(&node->line, &line);
              node = NULL;
            }
            delete (&line, cursor_pos);
          }
          // should probably refactor to not use goto, but, i mean, it
          // works...
          goto draw_line;
        }
        break;
      default:
        continue;
      }
    }

    // backspace
    if (curr_byte == ASCII_DEL) {
      if (cursor_pos > 0) {
        if (node != NULL) {
          line_copy(&node->line, &line);
          node = NULL;
        }
        const size_t bytes_removed = backspace(&line, cursor_pos);
        cursor_pos -= bytes_removed;
        fputs(ANSI_CURSOR_LEFT, stdout);
        goto draw_line;
      }

      continue;
    }

    if (node != NULL) {
      line_copy(&node->line, &line);
      node = NULL;
    }

    line_insert(&line, curr_byte, cursor_pos);
    cursor_pos++;

    if (!is_continuation_byte_utf8(curr_byte)) {
      fputs(ANSI_CURSOR_RIGHT, stdout);
    }

  draw_line:

    fputs(ANSI_CURSOR_POS_SAVE, stdout);

    fputs(ANSI_REMOVE_FULL_LINE, stdout);
    // reset cursor to start of line
    printf("\r");

    fputs(prompt, stdout);
    PRINT_LINE(line);

    fputs(ANSI_CURSOR_POS_RESTORE, stdout);
  }

  VECTOR_PUSH(line, '\0');

  printf("\n");

  return line.data;
}
