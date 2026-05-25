#ifndef ACTION_UTILS_H
#define ACTION_UTILS_H

#include "lib/buffer.h"
#include "types.h"

#define PUTS(str) (void)fputs(str, stdout)
#define FLUSH() (void)fflush(stdout)

void cursor_left(LineReader *reader);

void cursor_right(LineReader *reader);

void cursor_left_n(LineReader *reader, unsigned n);

void cursor_right_n(LineReader *reader, unsigned n);

void draw_active_buffer(LineReader *reader);

void update_active_buffer(LineReader *reader, Buffer *buffer);

void copy_hist_buf_if_needed(LineReader *reader);

size_t read_n_bytes(uint8_t *buf, size_t count);

uint8_t read_byte(void);

unsigned short get_terminal_width(void);

unsigned get_line_length(const LineReader *reader);

// move cursor to the start of the bottom row of the current line
// i.e.
//                    this is a line la la la <-- cursor here
// cursor now here -->keeps going
void cursor_to_bottom(const LineReader *reader);

#endif
