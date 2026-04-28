#ifndef LINE_READER_STRUCT_H
#define LINE_READER_STRUCT_H

#include <stddef.h>
#include <stdint.h>

#include "lib/buffer.h"

struct LineReader;

typedef int (*Action)(struct LineReader *);

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
  Action ctrl_delete;
  Action page_up;
  Action page_down;
  Action shift_tab;
  Action tab;
  Action end_of_file;
  Action backspace;
  Action ctrl_backspace;
  int (*insert)(struct LineReader *, uint8_t);
} ActionSet;

// a history node is a node in the linked list storing command history.
typedef struct HistoryNode {
  struct HistoryNode *p_next;
  struct HistoryNode *p_prev;
  // buffer containing a line from history, this buffer is read only, this
  // buffer is null terminated, and the length does not include the null
  // terminator
  Buffer line;
} HistoryNode;

struct LineReader {
  // history is a linked list where root is the oldest this in history, end in
  // the newest thing in history, and current is where the user is in history
  // (by pressing up and down).
  HistoryNode *history_root;
  HistoryNode *history_end;
  HistoryNode *history_curr;

  ActionSet acts;

  Buffer buffer;

  Buffer *active_buffer;
  // current cursor position as represented by an index in the buffer.
  size_t buffer_offset;
  // current cursor position as represented by the number of characters before
  // the cursor. remember, this is utf-8 land, not all characters are one byte.
  unsigned cursor_pos;

  char *prompt;
  // length of the prompt in characters, remember, in utf-8, not all characters
  // are one byte.
  unsigned prompt_length;
};

typedef struct LineReader LineReader;

#endif
