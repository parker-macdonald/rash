#ifndef LINE_READER_H
#define LINE_READER_H

#include <stdint.h>

#include "../vector.h"

typedef VECTOR(uint8_t) line_t;

/**
 * @brief reads a line interactively from stdin, the returned buffer does not
 * include the '\n' and is null-terminated.
 * @return a null-terminated buffer containing the line (without the '\n')
 */
const uint8_t *readline(void *);

/**
 * @brief clears the history of the line reader.
 */
void clear_history(void);

/**
 * @brief prints the current history of the line reader.
 * @param count the number of history items to print, if count is -1 all the
 * history is printed,
 */
void print_history(int count);

/**
 * @brief cleans up the line reader (this can be called multiple times without
 * issue).
 */
void line_reader_destroy(void);

#endif
