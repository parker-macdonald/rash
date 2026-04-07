#ifndef ANSI_H
#define ANSI_H

// start of ansi control sequence
#define ANSI_ESCAPE '\033'
// same as eof when canonical mode is disabled
#define ASCII_END_OF_TRANSMISSION '\4'
// sent by the terminal when backspace is pressed when canonical mode is
// disabled
#define ASCII_DEL '\177'

// ctrl+backspace for some reason
#define ASCII_END_TRANS_BLOCK '\27'

// delete the current line (cursor will not move)
#define ANSI_REMOVE_FULL_LINE "\033[2K"
#define ANSI_REMOVE_BELOW_CURSOR "\033[0J"

#define ANSI_CURSOR_UP "\033[A"
#define ANSI_CURSOR_UP_N(s) "\033["s"A"

#define ANSI_CURSOR_DOWN "\033[B"
#define ANSI_CURSOR_DOWN_N(s) "\033["s"B"

#define ANSI_CURSOR_RIGHT "\033[C"
#define ANSI_CURSOR_RIGHT_N(s) "\033["s"C"

#define ANSI_CURSOR_LEFT "\033[D"
#define ANSI_CURSOR_LEFT_N(s) "\033["s"D"

#define ANSI_CURSOR_HOME "\033[H"

#define ANSI_ERASE_SCREEN "\033[2J"

#define ANSI_INSERT_BLANK_CHAR "\033[@"

#define ANSI_DELETE_CHAR "\033[P"

// save current cursor position to be restored later
#define ANSI_CURSOR_POS_SAVE "\033[s"
// restore cursor position that was previously saved
#define ANSI_CURSOR_POS_RESTORE "\033[u"

#endif
