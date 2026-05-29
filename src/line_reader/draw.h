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

// the move_cursor_* functions modify cursor_pos
// the draw_cursor_* functions do not modify cursor_pos

void move_cursor_left(LineReader *reader);

void move_cursor_right(LineReader *reader);

void move_cursor_left_n(LineReader *reader, unsigned n);

void move_cursor_right_n(LineReader *reader, unsigned n);

// move cursor to the start of the line below the current buffer
// i.e.
//                    this is a line la la la <-- cursor here
//                    keeps going end
// cursor here now -->
void draw_cursor_post_line(const LineReader *reader);

void draw_cursor_at(const LineReader *reader, unsigned cursor_pos);

unsigned short get_terminal_width(void);

#endif
