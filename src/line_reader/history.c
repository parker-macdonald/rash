#include "history.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/vector.h"
#include "line_reader/types.h"

void history_clear(LineReader *reader) {
  HistoryNode *node = reader->history_top;

  while (node != NULL) {
    HistoryNode *next_node = node->p_next;

    VECTOR_DESTROY(node->line);
    free(node);

    node = next_node;
  }
  reader->history_top = NULL;
  reader->history_bottom = NULL;
  reader->history_curr = NULL;
}

void history_print(LineReader *reader, int count) {
  assert(count >= -1);

  if (count == 0) {
    return;
  }

  HistoryNode *node = reader->history_top;

  if (count == -1) {
    for (unsigned int i = 1; node != NULL; i++) {
      printf("%5u  %.*s\n", i, (int)node->line.length, (char *)node->line.data);
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
    node = reader->history_top;
    count = length;
  }

  for (unsigned int i = (unsigned int)count - 1; node != NULL; i--) {
    printf("%5u  %.*s\n", i, (int)node->line.length, (char *)node->line.data);
    node = node->p_next;
  }
}

void history_add(LineReader *reader) {
  HistoryNode *new_node = malloc(sizeof(HistoryNode));

  new_node->line = reader->buffer;

  new_node->p_next = NULL;
  new_node->p_prev = reader->history_bottom;
  if (reader->history_bottom != NULL) {
    reader->history_bottom->p_next = new_node;
  } else {
    reader->history_top = new_node;
  }
  reader->history_bottom = new_node;
}
