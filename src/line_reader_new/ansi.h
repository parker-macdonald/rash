#ifndef ANSI_H
#define ANSI_H

#include <stdint.h>

// start of ansi control sequence
extern const uint8_t ANSI_START_CHAR;
// same as eof when canonical mode is disabled
extern const uint8_t ASCII_END_OF_TRANSMISSION;
// sent by the terminal when backspace is pressed when canonical mode is
// disabled
extern const uint8_t ASCII_DEL;

// delete the current line (cursor will not move)
extern const char ANSI_REMOVE_FULL_LINE[];
extern const char ANSI_REMOVE_BELOW_CURSOR[];

extern const char ANSI_CURSOR_LEFT[];
extern const char ANSI_CURSOR_RIGHT[];

// save current cursor position to be restored later
extern const char ANSI_CURSOR_POS_SAVE[];
// restore cursor position that was previously saved
extern const char ANSI_CURSOR_POS_RESTORE[];

#endif
