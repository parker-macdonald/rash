#ifndef ACTIONS_ALL_H
#define ACTIONS_ALL_H

#include "line_reader/line_reader_struct.h"

int action_nop(LineReader *reader);

int action_clear(LineReader *reader);

int action_cursor_left(LineReader *reader);

int action_cursor_right(LineReader *reader);

int action_backspace(LineReader *reader);

int action_sigint(LineReader *reader);

int action_end_of_file(LineReader *reader);

int action_new_line(LineReader *reader);

int action_history_up(LineReader *reader);

int action_delete(LineReader *reader);

int action_insert(LineReader *reader, uint8_t byte);

#endif