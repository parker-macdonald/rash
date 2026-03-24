#ifndef LINE_READER_H
#define LINE_READER_H

#include <stdint.h>

// this is an opaque type, for the actual struct definition, see
// line_reader_struct.h
typedef struct line_reader line_reader;

line_reader *line_reader_create(void);

void line_reader_destroy2(line_reader *);

const uint8_t *line_reader_read(line_reader *);

// wrapper around line_reader_read that just takes a void ptr instead of a
// line_reader object. this is useful for the repl functions which take a reader
// function with a void ptr argument.
const uint8_t *line_reader_read_void(void *);

void line_reader_print_hist(line_reader *);

#endif
