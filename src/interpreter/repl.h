#ifndef REPL_H
#define REPL_H

#include <stdint.h>

int repl(const uint8_t *(*reader)(void *), void *reader_data);

int repl_once(const uint8_t *line);

#endif
