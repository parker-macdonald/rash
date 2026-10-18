#ifndef BUILTINS_BUILTINS_H
#define BUILTINS_BUILTINS_H

#include "builtins/trie.h"
#include "lib/buffer.h"

typedef struct {
  TrieNode *root;
} Builtins;

void builtins_init(Builtins *self);

void builtins_destroy(Builtins *self);

BuiltinFunc find_builtin(const Builtins *self, const char *str);

void find_matching_builtins(const Builtins *self, const Buffer *prefix, BufferList *list);

#endif
