#ifndef ACTIONS_ALL_H
#define ACTIONS_ALL_H

#include <stdint.h>

#include "readers/interactive_reader/types.h"

int action_nop(InteractiveReader *reader);

int action_clear(InteractiveReader *reader);

int action_cursor_left(InteractiveReader *reader);

int action_cursor_right(InteractiveReader *reader);

int action_backspace(InteractiveReader *reader);

int action_clear_line(InteractiveReader *reader);

int action_stop(InteractiveReader *reader);

int action_new_line(InteractiveReader *reader);

int action_history_up(InteractiveReader *reader);

int action_history_down(InteractiveReader *reader);

int action_delete(InteractiveReader *reader);

int action_insert(InteractiveReader *reader, uint8_t byte);

int action_word_left(InteractiveReader *reader);

int action_word_right(InteractiveReader *reader);

int action_home(InteractiveReader *reader);

int action_end(InteractiveReader *reader);

int action_complete(InteractiveReader *reader);

int action_delete_word_left(InteractiveReader *reader);

int action_delete_word_right(InteractiveReader *reader);

#endif
