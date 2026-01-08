#ifndef AUTO_COMPLETE_H
#define AUTO_COMPLETE_H

#include <stddef.h>

#include "lib/vec_types.h"

void get_file_matches(strings_t *matches, const char *word, size_t word_len);

void get_command_matches(strings_t *matches, const char *word, size_t word_len);

#endif
