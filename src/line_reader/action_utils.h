#ifndef ACTION_UTILS_H
#define ACTION_UTILS_H

#include "lib/buffer.h"
#include "line_reader_struct.h"

#define PUTS(str) (void)fputs(str, stdout)
#define FLUSH() (void)fflush(stdout)

// returned by getch when a sigint interrupted the read.
#define SIGINT_ON_READ (-1)

void cursor_left(LineReader *reader);

void cursor_right(LineReader *reader);

void cursor_left_n(LineReader *reader, unsigned n);

void cursor_right_n(LineReader *reader, unsigned n);

void draw_active_buffer(LineReader *reader);

void update_active_buffer(LineReader *reader, Buffer *buffer);

void copy_hist_buf_if_needed(LineReader *reader);

/**
 * @brief reads a byte from stdin without printing it to the screen or moving
 * the cursor.
 * @return returns a uint8_t casted to an int or RECV_SIGINT, when a sigint is
 * recieved.
 */
int getch(void);

unsigned short get_terminal_width(void);

unsigned get_line_length(const LineReader *reader);

// move cursor to the start of the bottom row of the current line
// i.e.
//                    this is a line la la la <-- cursor here
// cursor now here -->keeps going
void cursor_to_bottom(const LineReader *reader);

#endif
