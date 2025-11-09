#include "line_reader.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ansi.h"
#include "../utf_8.h"
#include "../vector.h"
#include "modify_line.h"
#include "utils.h"

#ifdef static_assert
static_assert(
    sizeof(char) == sizeof(uint8_t),
    "char is not one byte in size, god save you..."
);
#endif

typedef struct line_node {
  struct line_node *p_next;
  struct line_node *p_prev;
  line_t line;
} line_node_t;

static line_node_t *root_line_node = NULL;
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

void print_history(int count) {
  assert(count >= -1);

  if (count == 0) {
    return;
  }

  line_node_t *node = root_line_node;

  if (count == -1) {
    for (unsigned int i = 1; node != NULL; i++) {
      printf("%5u  ", i);
      PRINT_LINE(node->line);
      fputs("\n", stdout);
      node = node->p_next;
    }

    return;
  }

  // traverse to the last line
  int length;
  for (length = 1;; length++) {
    if (node->p_next == NULL) {
      break;
    }

    node = node->p_next;
  }

  if (count < length) {
    // backtrack the number of lines to print
    for (int i = 1; i < count; i++) {
      node = node->p_prev;
    }
  } else {
    node = root_line_node;
    count = length;
  }

  for (int i = count - 1; node != NULL; i--) {
    printf("%5d  ", length - i);
    PRINT_LINE(node->line);
    fputs("\n", stdout);
    node = node->p_next;
  }
}

#define CURSOR_RIGHT                                                           \
  do {                                                                         \
    displayed_cursor_pos++;                                                    \
    if (displayed_cursor_pos - 1 % width != 0 &&                               \
        displayed_cursor_pos % width == 0) {                                   \
      fputs("\r\033[B", stdout);                                               \
    } else {                                                                   \
      fputs(ANSI_CURSOR_RIGHT, stdout);                                        \
    }                                                                          \
  } while (0)

#define CURSOR_LEFT                                                            \
  do {                                                                         \
    displayed_cursor_pos--;                                                    \
    if ((displayed_cursor_pos + 1) % width == 0) {                             \
      fputs("\033[999C\033[Aa", stdout);                                       \
    } else {                                                                   \
      fputs(ANSI_CURSOR_LEFT, stdout);                                         \
    }                                                                          \
  } while (0)

#define DRAW_LINE(line)                                                        \
  do {                                                                         \
    width = get_terminal_width();                                              \
    if (characters_printed / width > 0) {                                      \
      printf("\033[%zuA", characters_printed / width);                         \
    }                                                                          \
    fputs(ANSI_REMOVE_BELOW_CURSOR, stdout);                                   \
    printf("\r");                                                              \
    fputs(prompt, stdout);                                                     \
    PRINT_LINE(line);                                                          \
  } while (0)

const uint8_t *readline(void) {
  char *prompt = getenv("PS1");
  if (prompt == NULL) {
    prompt = "$ ";
  }

  printf("%s", prompt);
  fflush(stdout);

  unsigned short width = get_terminal_width();
  const size_t prompt_length = strlen(prompt);
  size_t characters_printed = prompt_length;

  line_node_t *node = NULL;

  line_t line;
  VECTOR_INIT(line);

  size_t cursor_pos = 0;
  size_t displayed_cursor_pos = characters_printed;

  uint8_t curr_byte;
  for (;;) {
    int ch = getch();

    // this is sent by the sigint handler to let the line reader know the user
    // pressed ctrl-c to trigger a SIGINT.
    if (ch == SIGINT_ON_READ) {
      printf("\n%s", prompt);
      fflush(stdout);
      characters_printed = prompt_length;
      line.length = 0;
      cursor_pos = 0;
      continue;
    }

    curr_byte = (uint8_t)ch;

    if (curr_byte == ASCII_END_OF_TRANSMISSION) {
      printf("\n");
      VECTOR_DESTROY(line);
      line_reader_destroy();
      return NULL;
    }

    // readonly line to get length or data info from. when the user is going
    // through history, node contains the line they are viewing whereas line
    // contains the buffer the user is currently editting
    line_t *line_to_read = node == NULL ? &line : &node->line;

    if (curr_byte == '\n') {
      if (line_to_read->length != 0) {
        break;
      }

      printf("\n%s", prompt);
      fflush(stdout);
      continue;
    }

    if (curr_byte == ANSI_START_CHAR) {
      curr_byte = (uint8_t)getch();

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
              displayed_cursor_pos =
                  strlen_utf8(node->line.data, node->line.length) +
                  prompt_length;
              characters_printed = displayed_cursor_pos;
              DRAW_LINE(node->line);
              fflush(stdout);
            }
          } else if (node->p_prev != NULL) {
            node = node->p_prev;
            cursor_pos = node->line.length;
            displayed_cursor_pos =
                strlen_utf8(node->line.data, node->line.length) + prompt_length;
            characters_printed = displayed_cursor_pos;
            DRAW_LINE(node->line);
            fflush(stdout);
          }
          continue;
        // arrow down
        case 'B':
          if (node != NULL && node->p_next != NULL) {
            node = node->p_next;
            cursor_pos = node->line.length;
            displayed_cursor_pos =
                strlen_utf8(node->line.data, node->line.length) + prompt_length;
            characters_printed = displayed_cursor_pos;
            DRAW_LINE(node->line);
            fflush(stdout);
          } else {
            node = NULL;
            cursor_pos = line.length;
            displayed_cursor_pos =
                strlen_utf8(line.data, line.length) + prompt_length;
            characters_printed = displayed_cursor_pos;
            DRAW_LINE(line);
            fflush(stdout);
          }
          continue;
        // right arrow
        case 'C':
          if (cursor_pos < line_to_read->length) {
            cursor_pos += traverse_forward_utf8(
                line_to_read->data, line_to_read->length, cursor_pos
            );

            CURSOR_RIGHT;
            fflush(stdout);
          }
          continue;
        // left arrow
        case 'D':
          if (cursor_pos > 0) {
            cursor_pos -= traverse_back_utf8(line_to_read->data, cursor_pos);

            CURSOR_LEFT;
            fflush(stdout);
          }
          continue;
        case '1':
          if (getch() == ';') {
            if (getch() == '5') {
              const uint8_t arrow_char = (uint8_t)getch();
              // shift right arrow
              if (arrow_char == 'C') {
                if (cursor_pos < line_to_read->length) {
                  cursor_pos += traverse_forward_utf8(
                      line_to_read->data, line_to_read->length, cursor_pos
                  );
                  CURSOR_RIGHT;

                  while (cursor_pos <= line_to_read->length - 1 &&
                         line_to_read->data[cursor_pos] != ' ') {
                    cursor_pos += traverse_forward_utf8(
                        line_to_read->data, line_to_read->length, cursor_pos
                    );
                    CURSOR_RIGHT;
                  }

                  fflush(stdout);
                }

                continue;
              }
              // shift left arrow
              if (arrow_char == 'D') {
                if (cursor_pos > 0) {
                  cursor_pos -=
                      traverse_back_utf8(line_to_read->data, cursor_pos);
                  CURSOR_LEFT;

                  while (cursor_pos > 0 &&
                         line_to_read->data[cursor_pos - 1] != ' ') {
                    cursor_pos -=
                        traverse_back_utf8(line_to_read->data, cursor_pos);
                    CURSOR_LEFT;
                  }

                  fflush(stdout);
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
                line_copy(&line, &node->line);
                node = NULL;
              }
              line_delete(&line, cursor_pos);
              characters_printed--;
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
          line_copy(&line, &node->line);
          node = NULL;
        }
        const size_t bytes_removed = line_backspace(&line, cursor_pos);
        cursor_pos -= bytes_removed;
        characters_printed--;
        CURSOR_LEFT;
        goto draw_line;
      }

      continue;
    }

    if (node != NULL) {
      line_copy(&line, &node->line);
      node = NULL;
    }

    line_insert(&line, curr_byte, cursor_pos);
    cursor_pos++;

    if (!is_continuation_byte_utf8(curr_byte)) {
      CURSOR_RIGHT;
      characters_printed++;
    }

  draw_line:

    fputs(ANSI_CURSOR_POS_SAVE, stdout);

    DRAW_LINE(line);

    fputs(ANSI_CURSOR_POS_RESTORE, stdout);

    fflush(stdout);
  }

  line_node_t *new_node = malloc(sizeof(line_node_t));
  if (node != NULL) {
    node->line.length++;
    line_copy(&line, &node->line);
    node->line.length--;
    line.length--;
  } else {
    VECTOR_PUSH(line, '\0');
    line.length--;
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

  printf("\n");

  return line.data;
}
