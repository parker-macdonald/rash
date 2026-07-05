#ifndef HISTORY_H
#define HISTORY_H

#include "lib/buffer.h"
#include "line_reader/types.h"

void history_clear(LineReader *reader);

void history_print(LineReader *reader, int count);

void history_add(LineReader *reader);

Buffer *history_curr(LineReader *reader);

#endif
