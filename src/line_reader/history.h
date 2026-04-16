#ifndef HISTORY_H
#define HISTORY_H

#include "line_reader/line_reader_struct.h"

void history_clear(LineReader *);

void history_print(LineReader *reader, int count);

void history_add(LineReader *);

#endif
