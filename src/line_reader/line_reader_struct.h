#ifndef LINE_READER_STRUCT_H
#define LINE_READER_STRUCT_H

#include "lib/buffer.h"
#include "line_reader/actions.h"

// a history node is a node in the linked list storing command history.
typedef struct history_node_t {
  struct history_node_t *p_next;
  struct history_node_t *p_prev;
  // buffer containing a line from history, this buffer is read only, this
  // buffer is null terminated, and the length does not include the null
  // terminator
  Buffer line;
} HistoryNode;

typedef struct line_reader_t {
  // history is a linked list where root is the oldest this in history, end in
  // the newest thing in history, and current is where the user is in history
  // (by pressing up and down).
  HistoryNode *history_root;
  HistoryNode *history_end;
  HistoryNode *history_curr;

  Actions acts;

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
} LineReader;

#endif
