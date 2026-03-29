#ifndef ACTIONS_H
#define ACTIONS_H

#include <stdint.h>

// opaque stuct definition to prevent recursive includes
typedef struct line_reader_t LineReader;

typedef int (*Action)(LineReader *);

typedef struct {
  Action form_feed;
  Action sigint;
  Action arrow_left;
  Action arrow_right;
  Action delete;
  Action ctrl_left_arrow;
  Action ctrl_right_arrow;
  Action arrow_up;
  Action arrow_down;
  Action home;
  Action end;
  Action new_line;
  Action shift_delete;
  Action page_up;
  Action page_down;
  Action shift_tab;
  Action tab;
  Action end_of_file;
  Action backspace;
  Action shift_backspace;
  int (*insert)(LineReader *, uint8_t);
} Actions;

int preform_action(LineReader *);

void actions_default(Actions *);

#endif
