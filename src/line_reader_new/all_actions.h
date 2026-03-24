#ifndef ALL_ACTIONS_H
#define ALL_ACTIONS_H

#include "line_reader_new/line_reader.h"

int action_nop(line_reader *reader);

int action_clear(line_reader *reader);

int action_cursor_left(line_reader *reader);

int action_cursor_right(line_reader *reader);

int action_backspace(line_reader *reader);

int action_sigint(line_reader *reader);

int action_end_of_file(line_reader *reader);

int action_new_line(line_reader *reader);

int action_history_up(line_reader *reader);

int action_insert(line_reader *reader, uint8_t byte);


#endif