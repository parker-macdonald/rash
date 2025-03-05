#ifndef LINE_READER_H
#define LINE_READER_H

#include "vector.h"

typedef VECTOR(char) line_t;

char *readline(char *data, const char *const prompt);

#endif
