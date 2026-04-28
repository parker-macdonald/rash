#ifndef FIND_CMD_H
#define FIND_CMD_H

#include <stddef.h>

#include "lib/string.h"

typedef int (*builtin_t)(char **);

void trie_init(void);

void trie_destroy(void);

builtin_t find_builtin(const char *str);

void find_matching_builtins(const String *prefix, StringList *vec);

#endif
