#ifndef DRAW_H
#define DRAW_H

#include "types.h"

#define PUTS(str) (void)fputs(str, stdout)
#define FLUSH() (void)fflush(stdout)

unsigned get_line_width(const LineReader *reader);

// none of these functions flush so that you can chain them together

// draw entire state from the saved cursor position (see lib/ansi.h
// ANSI_CURSOR_SAVE / ANSI_CURSOR_RESTORE)
void draw_entire_state(const LineReader *reader);

void draw_cursor_left(const LineReader *reader);

void draw_cursor_right(const LineReader *reader);

void draw_cursor_left_n(const LineReader *reader, unsigned n);

void draw_cursor_right_n(const LineReader *reader, unsigned n);

// move cursor to the start of the line below the current buffer
// i.e.
//                    this is a line la la la <-- cursor here
//                    keeps going end
// cursor here now -->
void draw_cursor_post_line(const LineReader *reader);

unsigned short get_terminal_width(void);

#endif
