#include "actions.h"

#include <stdint.h>
#include <string.h>

#include "lib/ansi.h"
#include "readers/interactive_reader/action_utils.h"
#include "readers/interactive_reader/actions_all.h"
#include "readers/interactive_reader/types.h"

#define CTRL_A 1
#define CTRL_B 2
#define CTRL_C 3
#define CTRL_D 4
#define CTRL_E 5
#define CTRL_F 6
#define CTRL_G 7
#define CTRL_H 8
#define CTRL_I 9
#define CTRL_J 10
#define CTRL_K 11
#define CTRL_L 12
#define CTRL_M 13
#define CTRL_N 14
#define CTRL_O 15
#define CTRL_P 16
#define CTRL_Q 17
#define CTRL_R 18
#define CTRL_S 19
#define CTRL_T 20
#define CTRL_U 21
#define CTRL_V 22
#define CTRL_W 23
#define CTRL_X 24
#define CTRL_Y 25
#define CTRL_Z 26

int preform_action(InteractiveReader *reader) {
  uint8_t byte = read_byte();

  if (byte == '\n' || byte == '\r') {
    return reader->action_set.new_line(reader);
  }

  if (byte == ANSI_ESCAPE) {
    char seq[16] = {0};

    read_n_bytes((uint8_t *)seq, 15);

    if (strcmp(seq, "d") == 0) {
      return reader->action_set.ctrl_delete(reader);
    }

    if (strcmp(seq, "[A") == 0) {
      return reader->action_set.arrow_up(reader);
    }

    if (strcmp(seq, "[B") == 0) {
      return reader->action_set.arrow_down(reader);
    }

    if (strcmp(seq, "[C") == 0) {
      return reader->action_set.arrow_right(reader);
    }

    if (strcmp(seq, "[D") == 0) {
      return reader->action_set.arrow_left(reader);
    }

    if (strcmp(seq, "[H") == 0) {
      return reader->action_set.home(reader);
    }

    if (strcmp(seq, "[F") == 0) {
      return reader->action_set.end(reader);
    }

    if (strcmp(seq, "[Z") == 0) {
      return reader->action_set.shift_tab(reader);
    }

    if (strcmp(seq, "[1;5C") == 0) {
      return reader->action_set.ctrl_right_arrow(reader);
    }

    if (strcmp(seq, "[1;5D") == 0) {
      return reader->action_set.ctrl_left_arrow(reader);
    }

    if (strcmp(seq, "[3~") == 0) {
      return reader->action_set.delete(reader);
    }

    if (strcmp(seq, "[5~") == 0) {
      return reader->action_set.page_up(reader);
    }

    if (strcmp(seq, "[6~") == 0) {
      return reader->action_set.page_down(reader);
    }
  }

  if (byte == ASCII_DEL) {
    return reader->action_set.backspace(reader);
  }

  // tab
  if (byte == '\t') {
    return reader->action_set.tab(reader);
  }

  if (byte == CTRL_A) {
    return reader->action_set.ctrl_a(reader);
  }

  if (byte == CTRL_B) {
    return reader->action_set.ctrl_b(reader);
  }

  if (byte == CTRL_C) {
    return reader->action_set.ctrl_c(reader);
  }

  if (byte == CTRL_D) {
    return reader->action_set.ctrl_d(reader);
  }

  if (byte == CTRL_E) {
    return reader->action_set.ctrl_e(reader);
  }

  if (byte == CTRL_F) {
    return reader->action_set.ctrl_f(reader);
  }

  if (byte == CTRL_G) {
    return reader->action_set.ctrl_g(reader);
  }

  if (byte == CTRL_H) {
    return reader->action_set.ctrl_h(reader);
  }

  if (byte == CTRL_I) {
    return reader->action_set.ctrl_i(reader);
  }

  if (byte == CTRL_J) {
    return reader->action_set.ctrl_j(reader);
  }

  if (byte == CTRL_K) {
    return reader->action_set.ctrl_k(reader);
  }

  if (byte == CTRL_L) {
    return reader->action_set.ctrl_l(reader);
  }

  if (byte == CTRL_M) {
    return reader->action_set.ctrl_m(reader);
  }

  if (byte == CTRL_N) {
    return reader->action_set.ctrl_n(reader);
  }

  if (byte == CTRL_O) {
    return reader->action_set.ctrl_o(reader);
  }

  if (byte == CTRL_P) {
    return reader->action_set.ctrl_p(reader);
  }

  if (byte == CTRL_Q) {
    return reader->action_set.ctrl_q(reader);
  }

  if (byte == CTRL_R) {
    return reader->action_set.ctrl_r(reader);
  }

  if (byte == CTRL_S) {
    return reader->action_set.ctrl_s(reader);
  }

  if (byte == CTRL_T) {
    return reader->action_set.ctrl_t(reader);
  }

  if (byte == CTRL_U) {
    return reader->action_set.ctrl_u(reader);
  }

  if (byte == CTRL_V) {
    return reader->action_set.ctrl_v(reader);
  }

  if (byte == CTRL_W) {
    return reader->action_set.ctrl_w(reader);
  }

  if (byte == CTRL_X) {
    return reader->action_set.ctrl_x(reader);
  }

  if (byte == CTRL_Y) {
    return reader->action_set.ctrl_y(reader);
  }

  if (byte == CTRL_Z) {
    return reader->action_set.ctrl_z(reader);
  }

  // check if byte is an ascii control character
  if (byte <= 31) {
    return 0;
  }

  reader->action_set.insert(reader, byte);

  return 0;
}

void actions_default(ActionSet *acts) {
  acts->arrow_left = action_cursor_left;
  acts->arrow_right = action_cursor_right;
  acts->delete = action_delete;
  acts->ctrl_left_arrow = action_word_left;
  acts->ctrl_right_arrow = action_word_right;
  acts->arrow_up = action_history_up;
  acts->arrow_down = action_history_down;
  acts->home = action_home;
  acts->end = action_end;
  acts->new_line = action_new_line;
  acts->ctrl_delete = action_delete_word_right;
  acts->page_up = action_nop;
  acts->page_down = action_nop;
  acts->shift_tab = action_nop;
  acts->tab = action_complete;
  acts->insert = action_insert;
  acts->backspace = action_backspace;

  acts->ctrl_d = action_stop;
  acts->ctrl_l = action_clear;
  acts->ctrl_c = action_clear_line;
  // on gnome terminal ctrl+backspace is the same as ctrl+w
  acts->ctrl_w = action_delete_word_left;
  // on kde's konsole ctrl+backspace is the same as ctrl+h
  acts->ctrl_h = action_delete_word_left;

  acts->ctrl_a = action_nop;
  acts->ctrl_b = action_nop;
  acts->ctrl_e = action_nop;
  acts->ctrl_f = action_nop;
  acts->ctrl_g = action_nop;
  acts->ctrl_i = action_nop;
  acts->ctrl_j = action_nop;
  acts->ctrl_k = action_nop;
  acts->ctrl_m = action_nop;
  acts->ctrl_n = action_nop;
  acts->ctrl_o = action_nop;
  acts->ctrl_p = action_nop;
  acts->ctrl_q = action_nop;
  acts->ctrl_r = action_nop;
  acts->ctrl_s = action_nop;
  acts->ctrl_t = action_nop;
  acts->ctrl_u = action_nop;
  acts->ctrl_v = action_nop;
  acts->ctrl_x = action_nop;
  acts->ctrl_y = action_nop;
  acts->ctrl_z = action_nop;
}
