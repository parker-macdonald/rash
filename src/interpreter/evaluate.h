#ifndef PARSER_H
#define PARSER_H

#include "interpreter/token.h"

typedef struct {
  const TokenList *tokens;
  size_t current;
} EvalState;

int evaluate(const TokenList *tokens);

#endif
