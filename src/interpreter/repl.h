#ifndef REPL_H
#define REPL_H

#include "readers/generic_reader.h"

int repl(GenericReader *reader);

int repl_once(const Buffer *line);

#endif
