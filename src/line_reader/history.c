#include "history.h"

#include <stdio.h>

#include "line_reader/line_reader_struct.h"

void history_clear(LineReader *reader) {
  HistoryNode *node = reader->history_root;

  while (node != NULL) {
    HistoryNode *next_node = node->p_next;

    VECTOR_DESTROY(node->line);
    free(node);

    node = next_node;
  }
  reader->history_root = NULL;
  reader->history_end = NULL;
  reader->history_curr = NULL;
}

void history_print(LineReader *reader, int count) {
  assert(count >= -1);

  if (count == 0) {
    return;
  }

  HistoryNode *node = reader->history_root;

  if (count == -1) {
    for (unsigned int i = 1; node != NULL; i++) {
      printf("%5u  %s\n", i, (char *)node->line.data);
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
    node = reader->history_root;
    count = length;
  }

  for (unsigned int i = (unsigned int)count - 1; node != NULL; i--) {
    printf("%5u  %s\n", i, (char *)node->line.data);
    node = node->p_next;
  }
}

void history_add(LineReader *reader) {
  HistoryNode *new_node = malloc(sizeof(HistoryNode));

  new_node->line = reader->buffer;
  // since the length doesn't include the null terminator, we must subtract one
  new_node->line.length--;

  new_node->p_next = NULL;
  new_node->p_prev = reader->history_end;
  if (reader->history_end != NULL) {
    reader->history_end->p_next = new_node;
  } else {
    reader->history_root = new_node;
  }
  reader->history_end = new_node;
}
