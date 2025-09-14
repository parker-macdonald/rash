#ifndef LEXER_H
#define LEXER_H

#include "stdint.h"

#include "execute.h"
#include "optional.h"

typedef OPTIONAL(execution_context) optional_exec_context;

optional_exec_context get_tokens_from_line(const uint8_t *const line);

#endif
