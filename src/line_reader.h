#ifndef LINE_READER_H
#define LINE_READER_H

#include "vector.h"
#include <stdint.h>

typedef VECTOR(uint8_t) line_t;

uint8_t *readline(uint8_t *data, const char *const prompt);

#endif
