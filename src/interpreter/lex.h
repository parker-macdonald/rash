#ifndef LEX_H
#define LEX_H

#include "lib/buffer.h"
#include "interpreter/token.h"

TokenList lex(const Buffer *source);

#endif
