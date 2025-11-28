#ifndef REPL_H
#define REPL_H

#include <stdint.h>

int repl(const uint8_t *(*reader)(void));

#endif
