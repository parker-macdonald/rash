#ifndef ALIAS_H
#define ALIAS_H

#include <stddef.h>

#include "interpreter/lex.h"

void alias_set(const char *key, token_t *tokens);

token_t *alias_get(const char *key);

int alias_unset(const char *key);

void alias_print(void);

#endif
