#ifndef LINE_READER_H
#define LINE_READER_H

#include "vector.h"
#include <stdint.h>

typedef VECTOR(uint8_t) line_t;

uint8_t *readline(const char *const prompt);

void line_reader_destroy(void);

#endif
