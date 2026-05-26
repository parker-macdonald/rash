#ifndef REPL_H
#define REPL_H

#include "lib/buffer.h"

int repl(const Buffer *(*reader)(void *), void *reader_data);

int repl_once(const Buffer *line);

#endif
