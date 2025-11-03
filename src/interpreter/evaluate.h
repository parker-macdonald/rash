#ifndef PARSER_H
#define PARSER_H

#include "../vector.h"
#include "lex.h"

typedef VECTOR(char *) argv_t;

int evaluate(const token_t *tokens);

#endif
