#ifndef AUTO_COMPLETE_H
#define AUTO_COMPLETE_H

#include <stddef.h>

#include "lib/vec_types.h"

size_t get_matches(StringList *matches, Buffer *line, size_t cursor_pos);

void get_file_matches(StringList *matches, const char *word, size_t word_len);

void get_command_matches(StringList *matches, const char *word, size_t word_len);

#endif
