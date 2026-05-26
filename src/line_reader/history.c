#include "history.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/vector.h"
#include "line_reader/types.h"

void history_clear(LineReader *reader) {
  VECTOR_DESTROY(reader->history);

  reader->history._capacity = 0;
  reader->history.length = 0;
  reader->history.data = NULL;

  reader->history_curr = 0;
}

void history_print(LineReader *reader, int count) {
  assert(count >= -1);

  if (count == 0) {
    return;
  }

  HistoryNode *node = reader->history_begin;

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
    node = reader->history_begin;
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
    reader->history_begin = new_node;
  }
  reader->history_end = new_node;
}

Buffer *history_curr(LineReader *reader) {
  assert(reader->history_curr < reader->history.length);

  return &reader->history.data[reader->history_curr];
}

