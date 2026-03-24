#ifndef HISTORY_H
#define HISTORY_H

#include "line_reader_new/line_reader.h"

void history_clear(line_reader *);

void history_print(line_reader *reader, int count);

void history_add(line_reader *);

#endif
