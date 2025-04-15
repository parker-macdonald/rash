#ifndef LINE_READER_H
#define LINE_READER_H

#include "vector.h"
#include <stdint.h>

typedef VECTOR(uint8_t) line_t;

typedef struct line_node {
  struct line_node *p_next;
  struct line_node *p_prev;
  line_t line;
} line_node_t;

extern line_node_t *root_line_node;

#define PRINT_LINE(line)                                                       \
  fwrite((line).data, sizeof(*(line).data), (line).length, stdout)

const uint8_t *readline(const char *const prompt);

void clear_history(void);

void line_reader_destroy(void);

#endif
