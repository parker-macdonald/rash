#include "line_reader.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../line_reader/utils.h"
#include "../vector.h"
#include "ansi.h"
#include "modify_line.h"
#include "utf_8.h"

size_t handle_ansi_seq(line_t *const line);

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

  line_t line;
  VECTOR_INIT(line);
  size_t cursor_pos = 0;

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
      VECTOR_CLEAR(line);
      cursor_pos = 0;
      printf("\n%s", prompt);
      fflush(stdout);
      continue;
    }

    if (character == ASCII_END_OF_TRANSMISSION) {
      VECTOR_DESTROY(line);
      return NULL;
    }

    if (character == '\n') {
      break;
    }

    // in this switch statement, continuing will not draw the line, breaking
    // will.
    switch (state) {
      case DEFAULT:
        if (character == ANSI_START_CHAR) {
          state = BEGIN_ANSI_SEQ;
          break;
        }

        if (character == ASCII_DEL) {
          if (cursor_pos -= line_backspace(&line, cursor_pos)) {
            fputs(ANSI_CURSOR_LEFT, stdout);
          }

          break;
        }

        line_insert(&line, (uint8_t)character, cursor_pos);
        fputs(ANSI_CURSOR_RIGHT, stdout);
        cursor_pos++;
        break;

      case BEGIN_ANSI_SEQ:
        state = character == '[' ? ANSI_SEQ_CONT : DEFAULT;
        continue;

      case ANSI_SEQ_CONT:
        if (character == 'A') {
          // TODO: arrow up
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
          const size_t count =
              traverse_forward_utf8(line.data, line.length, cursor_pos);
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
          const size_t count = traverse_back_utf8(line.data, cursor_pos);
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
          line_delete(&line, cursor_pos);
          break;
        }

        continue;

      case ANSI_CTRL_ARROW:
        // right arrow
        if (character == 'C') {
          while (cursor_pos < line.length) {
            cursor_pos++;
            fputs(ANSI_CURSOR_RIGHT, stdout);

            if (line.data[cursor_pos] == ' ') {
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

            if (line.data[cursor_pos] == ' ') {
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
    PRINT_LINE(line);

    fputs(ANSI_CURSOR_POS_RESTORE, stdout);

    fflush(stdout);
  }

  printf("\n");
  VECTOR_PUSH(line, '\0');
  return line.data;
}
