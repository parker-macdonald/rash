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
  ANSI_SEQ_CONT_3,
  // when the user presses the delete character
  ANSI_DELETE
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

    switch (state) {
      case DEFAULT:
        if (character == ANSI_START_CHAR) {
          state = BEGIN_ANSI_SEQ;
          break;
        }

        if (character == ASCII_DEL) {
          if (cursor_pos -= line_backspace(&line, cursor_pos)) {
            fputs(ANSI_CURSOR_LEFT, stdout);
            fflush(stdout);
          }

          break;
        }

        line_insert(&line, (uint8_t)character, cursor_pos);
        cursor_pos++;
        printf("%c", (char)character);
        fflush(stdout);
        break;

      case BEGIN_ANSI_SEQ:
        state = character == '[' ? ANSI_SEQ_CONT : DEFAULT;
        break;

      case ANSI_SEQ_CONT:
        if (character == 'A') {
          // TODO: arrow up
          state = DEFAULT;
          break;
        }

        if (character == 'B') {
          // TODO: arrow down
          state = DEFAULT;
          break;
        }

        // right arrow
        if (character == 'C') {
          if (cursor_pos +=
              traverse_forward_utf8(line.data, line.length, cursor_pos)) {
            fputs(ANSI_CURSOR_RIGHT, stdout);
            fflush(stdout);
          }

          state = DEFAULT;
          break;
        }

        // left arrow
        if (character == 'D') {
          if (cursor_pos -= traverse_back_utf8(line.data, cursor_pos)) {
            fputs(ANSI_CURSOR_LEFT, stdout);
            fflush(stdout);
          }

          state = DEFAULT;
          break;
        }

        if (character == '1') {
          state = ANSI_SEQ_CONT_1;
          break;
        }

        if (character == '3') {
          state = ANSI_SEQ_CONT_3;
          break;
        }

      case ANSI_SEQ_CONT_1:
        state = character == ';' ? ANSI_SEQ_CONT_1_SEMI : DEFAULT;
        break;

      case ANSI_SEQ_CONT_1_SEMI:
        state = character == '5' ? ANSI_CTRL_ARROW : DEFAULT;
        break;

      case ANSI_SEQ_CONT_3:
        state = character == '~' ? ANSI_DELETE : DEFAULT;
        break;

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
          break;
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
          break;
        }

      case ANSI_DELETE:
        line_delete(&line, cursor_pos);
        state = DEFAULT;
        break;
    }
  }

  printf("\n");
  VECTOR_PUSH(line, '\0');
  return line.data;
}
