#ifndef LINE_READER_H
#define LINE_READER_H

#include <stdint.h>

#include "lib/buffer.h"

void line_reader_init(void);

void line_reader_destroy(void);

const Buffer *line_reader_read(void);

// wrapper around line_reader_read that just takes a void ptr instead of
// nothing. this is useful for the repl functions which take a reader function
// with a void ptr argument.
const Buffer *line_reader_read_void(void *);

void line_reader_hist_print(int count);

void line_reader_hist_clear(void);

#endif
