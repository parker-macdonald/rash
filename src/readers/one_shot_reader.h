#ifndef ONE_SHOT_READER_H
#define ONE_SHOT_READER_H

#include "lib/buffer.h"
#include "readers/generic_reader.h"

// takes ownership of line
void one_shot_reader_create(GenericReader *out, Buffer line);

#endif
