#include "actions.h"

#include <ctype.h>
#include <stdint.h>

#include "lib/ansi.h"
#include "line_reader/action_utils.h"
#include "line_reader/actions_all.h"
#include "line_reader/line_reader_struct.h"

int preform_action(LineReader *reader) {
  int ch = getch();

  if (ch == SIGINT_ON_READ) {
    reader->acts.sigint(reader);
    return 0;
  }

  uint8_t byte = (uint8_t)ch;

  if (byte == ASCII_END_OF_TRANSMISSION) {
    reader->acts.end_of_file(reader);
    return -1;
  }

  if (byte == '\n' || byte == '\r') {
    if (reader->acts.new_line(reader)) {
      return 1;
    }
  }

  if (byte == ANSI_ESCAPE) {
    byte = (uint8_t)getch();

    if (byte == 'd') {
      reader->acts.shift_delete(reader);
      return 0;
    }

    if (byte != '[') {
      return 0;
    }

    switch (getch()) {
      // arrow up
      case 'A':
        reader->acts.arrow_up(reader);
        return 0;

      // arrow down
      case 'B':
        reader->acts.arrow_down(reader);
        return 0;

      // right arrow
      case 'C':
        reader->acts.arrow_right(reader);
        return 0;

      // left arrow
      case 'D':
        reader->acts.arrow_left(reader);
        return 0;

      case '1':
        if (getch() == ';') {
          if (getch() == '5') {
            const uint8_t arrow_char = (uint8_t)getch();
            // ctrl right arrow
            if (arrow_char == 'C') {
              reader->acts.ctrl_right_arrow(reader);
            }
            // ctrl left arrow
            if (arrow_char == 'D') {
              reader->acts.ctrl_left_arrow(reader);
            }
          }
        }
        return 0;

      case '3':
        // delete key
        if (getch() == '~') {
          reader->acts.delete(reader);
        }
        return 0;

      case '5':
        // page up
        if (getch() == '~') {
          reader->acts.page_up(reader);
          return 0;
        }
        return 0;

      case '6':
        // page down
        if (getch() == '~') {
          reader->acts.page_down(reader);
          return 0;
        }
        return 0;

      // home key
      case 'H':
        reader->acts.home(reader);
        return 0;

      // end key
      case 'F':
        reader->acts.end(reader);
        return 0;

      // shift+tab
      case 'Z':
        reader->acts.shift_tab(reader);
        return 0;

      default:
        break;
    }
  }

  if (byte == ASCII_DEL) {
    reader->acts.backspace(reader);
    return 0;
  }

  // tab
  if (byte == '\t') {
    reader->acts.tab(reader);
    return 0;
  }

  // ctrl+l (usually clears the screen)
  if (byte == '\f') {
    reader->acts.form_feed(reader);
    return 0;
  }

  if (iscntrl((int)byte)) {
    return 0;
  }

  reader->acts.insert(reader, byte);

  return 0;
}

void actions_default(Actions *acts) {
  acts->form_feed = action_clear;
  acts->sigint = action_sigint;
  acts->arrow_left = action_cursor_left;
  acts->arrow_right = action_cursor_right;
  acts->delete = action_delete;
  acts->ctrl_left_arrow = action_word_left;
  acts->ctrl_right_arrow = action_word_right;
  acts->arrow_up = action_history_up;
  acts->arrow_down = action_nop;
  acts->home = action_nop;
  acts->end = action_nop;
  acts->new_line = action_new_line;
  acts->shift_delete = action_nop;
  acts->page_up = action_nop;
  acts->page_down = action_nop;
  acts->shift_tab = action_nop;
  acts->tab = action_nop;
  acts->end_of_file = action_end_of_file;
  acts->insert = action_insert;
  acts->backspace = action_backspace;
  acts->shift_backspace = action_nop;
}
