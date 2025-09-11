#ifndef LEXER_H
#define LEXER_H

#include "stdint.h"

#include "execute.h"

execution_context get_tokens_from_line(const uint8_t *const line);

#endif
