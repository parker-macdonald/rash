#ifndef ACTIONS_H
#define ACTIONS_H

#include "line_reader_new/line_reader.h"

typedef int (*action_t)(line_reader *);

typedef struct {
  action_t form_feed;
  action_t sigint;
  action_t arrow_left;
  action_t arrow_right;
  action_t delete;
  action_t ctrl_left_arrow;
  action_t ctrl_right_arrow;
  action_t arrow_up;
  action_t arrow_down;
  action_t home;
  action_t end;
  action_t new_line;
  action_t shift_delete;
  action_t page_up;
  action_t page_down;
  action_t shift_tab;
  action_t tab;
  action_t end_of_file;
  action_t backspace;
  action_t shift_backspace;
  int (*insert)(line_reader *, uint8_t);
} actions;

int preform_action(line_reader *);

void actions_default(actions *);

#endif
