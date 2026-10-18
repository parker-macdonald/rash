#ifndef HISTORY_H
#define HISTORY_H

#include "lib/buffer.h"
#include "readers/interactive_reader/types.h"

void history_clear(InteractiveReader *reader);

void history_print(const InteractiveReader *reader, int count);

void history_add(InteractiveReader *reader);

Buffer *history_curr(const InteractiveReader *reader);

#endif
