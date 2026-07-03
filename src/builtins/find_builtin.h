#ifndef FIND_CMD_H
#define FIND_CMD_H

#include <stddef.h>

#include "lib/cstrlist.h"

typedef int (*builtin_t)(char **);

void trie_init(void);

void trie_destroy(void);

builtin_t find_builtin(const char *str);

void find_matching_builtins(
    const char *prefix, size_t prefix_len, CStrList *vec
);

#endif
