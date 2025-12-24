#ifndef AUTO_COMPLETE_H
#define AUTO_COMPLETE_H

#include <stddef.h>

#include "line_reader.h"

size_t auto_complete(line_t *line, size_t cursor_pos);

#endif
