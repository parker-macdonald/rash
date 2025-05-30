#include "line_reader.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../line_reader/utils.h"
#include "../vector.h"
#include "ansi.h"
#include "modify_line.h"
#include "utf_8.h"

#ifdef static_assert
static_assert(sizeof(char) == sizeof(uint8_t),
              "char is not one byte in size, god save you...");
#endif

#define MODIFY_LINE(code)                                                      \
  if (history_node != NULL) {                                                  \
    line_copy(&current_line, line_ptr);                                        \
    history_node = NULL;                                                       \
  }                                                                            \
  do                                                                           \
  code while (0)

typedef struct line_node {
  struct line_node *p_next;
  struct line_node *p_prev;
  line_t line;
} line_node_t;

static line_node_t *root_line_node = NULL;
static line_node_t *last_line_node = NULL;

enum reader_state {
  DEFAULT,
  // this state checks for the `[` character that is present in all ansi
  // sequences
  BEGIN_ANSI_SEQ,
  // this state checks for the charcter after the `[`
  ANSI_SEQ_CONT,
  // this state happens when the character after the `[` was found to be the
  // number 1
  ANSI_SEQ_CONT_1,
  ANSI_SEQ_CONT_1_SEMI,
  // when the user presses ctrl then an arrow key to move a full word
  ANSI_CTRL_ARROW,
  // this state happens when the character after the `[` was found to be the
  // number 3
  ANSI_SEQ_CONT_3
};

const uint8_t *readline(void) {
  const char *prompt = getenv("PS1");

  line_t current_line;
  VECTOR_INIT(current_line);
  size_t cursor_pos = 0;

  line_node_t *history_node = NULL;

  enum reader_state state = DEFAULT;

  if (prompt == NULL) {
    prompt = "$ ";
  }

  fputs(prompt, stdout);
  fflush(stdout);

  int character;
  for (;;) {
    character = getch();

    if (character == SIGINT_ON_READ) {
      history_node = NULL;
      VECTOR_CLEAR(current_line);
      cursor_pos = 0;
      printf("\n%s", prompt);
      fflush(stdout);
      continue;
    }

    if (character == ASCII_END_OF_TRANSMISSION) {
      VECTOR_DESTROY(current_line);
      return NULL;
    }

    if (character == '\n') {
      break;
    }

    line_t *line_ptr =
        history_node == NULL ? &current_line : &history_node->line;

    // in this switch statement, continuing will not draw the line, breaking
    // will.
    switch (state) {
      case DEFAULT:
        if (character == ANSI_START_CHAR) {
          state = BEGIN_ANSI_SEQ;
          break;
        }

        if (character == ASCII_DEL) {
          const size_t count = line_backspace(line_ptr, cursor_pos);
          if (count) {
            fputs(ANSI_CURSOR_LEFT, stdout);
            cursor_pos -= count;
          }

          break;
        }

        MODIFY_LINE({ line_insert(line_ptr, (uint8_t)character, cursor_pos); });
        fputs(ANSI_CURSOR_RIGHT, stdout);
        cursor_pos++;
        break;

      case BEGIN_ANSI_SEQ:
        state = character == '[' ? ANSI_SEQ_CONT : DEFAULT;
        continue;

      case ANSI_SEQ_CONT:
        // arrow up
        if (character == 'A') {
          if (history_node != NULL) {
            if (history_node->p_prev != NULL) {
              history_node = history_node->p_prev;
            }
          } else {
            history_node = last_line_node;
          }

          if (history_node != NULL) {
            draw_line(prompt, &history_node->line);
          }
          state = DEFAULT;
          continue;
        }

        if (character == 'B') {
          // TODO: arrow down
          state = DEFAULT;
          continue;
        }

        // right arrow
        if (character == 'C') {
          const size_t count = traverse_forward_utf8(
              line_ptr->data, line_ptr->length, cursor_pos);
          if (count) {
            cursor_pos += count;
            fputs(ANSI_CURSOR_RIGHT, stdout);
            fflush(stdout);
          }

          state = DEFAULT;
          continue;
        }

        // left arrow
        if (character == 'D') {
          const size_t count = traverse_back_utf8(line_ptr->data, cursor_pos);
          if (count) {
            cursor_pos -= count;
            fputs(ANSI_CURSOR_LEFT, stdout);
            fflush(stdout);
          }

          state = DEFAULT;
          continue;
        }

        if (character == '1') {
          state = ANSI_SEQ_CONT_1;
          continue;
        }

        if (character == '3') {
          state = ANSI_SEQ_CONT_3;
          continue;
        }

      case ANSI_SEQ_CONT_1:
        state = character == ';' ? ANSI_SEQ_CONT_1_SEMI : DEFAULT;
        continue;

      case ANSI_SEQ_CONT_1_SEMI:
        state = character == '5' ? ANSI_CTRL_ARROW : DEFAULT;
        continue;

      case ANSI_SEQ_CONT_3:
        state = DEFAULT;
        if (character == '~') {
          MODIFY_LINE({ line_delete(&current_line, cursor_pos); });
          break;
        }

        continue;

      case ANSI_CTRL_ARROW:
        // right arrow
        if (character == 'C') {
          while (cursor_pos < line_ptr->length) {
            cursor_pos++;
            fputs(ANSI_CURSOR_RIGHT, stdout);

            if (line_ptr->data[cursor_pos] == ' ') {
              break;
            }
          }

          fflush(stdout);

          state = DEFAULT;
          continue;
        }

        // left arrow
        if (character == 'D') {
          while (cursor_pos > 0) {
            cursor_pos--;
            fputs(ANSI_CURSOR_LEFT, stdout);

            if (line_ptr->data[cursor_pos] == ' ') {
              break;
            }
          }

          fflush(stdout);

          state = DEFAULT;
          continue;
        }
    }

    fputs(ANSI_CURSOR_POS_SAVE, stdout);

    fputs(ANSI_REMOVE_FULL_LINE, stdout);
    // reset cursor to start of line
    printf("\r");

    fputs(prompt, stdout);
    PRINT_LINE(*line_ptr);

    fputs(ANSI_CURSOR_POS_RESTORE, stdout);

    fflush(stdout);
  }

  line_node_t *new_node = malloc(sizeof(line_node_t));
  if (history_node != NULL) {
    line_copy(&current_line, &history_node->line);
  } else {
    VECTOR_PUSH(current_line, '\0');
  }

  new_node->line = current_line;

  new_node->p_next = NULL;
  new_node->p_prev = last_line_node;
  if (last_line_node != NULL) {
    last_line_node->p_next = new_node;
  } else {
    root_line_node = new_node;
  }
  last_line_node = new_node;

  printf("\n");
  return current_line.data;
}
