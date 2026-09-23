#ifndef LINE_READER_H
#define LINE_READER_H

#include "lib/buffer.h"
#include "readers/interactive_reader/types.h"

void interactive_reader_init(InteractiveReader *self);

void interactive_reader_destroy(InteractiveReader *self);

const Buffer *interactive_reader_read(InteractiveReader *self);

void interactive_reader_hist_print(const InteractiveReader *self, int count);

void interactive_reader_hist_clear(InteractiveReader *self);

#endif
