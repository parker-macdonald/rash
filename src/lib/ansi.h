#ifndef ANSI_H
#define ANSI_H

#include <stdint.h>

// start of ansi control sequence
extern const uint8_t ANSI_ESCAPE;
// same as eof when canonical mode is disabled
extern const uint8_t ASCII_END_OF_TRANSMISSION;
// sent by the terminal when backspace is pressed when canonical mode is
// disabled
extern const uint8_t ASCII_DEL;

// delete the current line (cursor will not move)
#define ANSI_REMOVE_FULL_LINE "\033[2K"
#define ANSI_REMOVE_BELOW_CURSOR "\033[0J";

#define ANSI_CURSOR_UP "\033[A"
#define ANSI_CURSOR_DOWN "\033[B"
#define ANSI_CURSOR_RIGHT "\033[C"
#define ANSI_CURSOR_LEFT "\033[D"

// save current cursor position to be restored later
#define ANSI_CURSOR_POS_SAVE "\033[s"
// restore cursor position that was previously saved
#define ANSI_CURSOR_POS_RESTORE "\033[u"

#endif
