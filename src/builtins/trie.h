#ifndef BUILTINS_TRIE_H
#define BUILTINS_TRIE_H

#include <stddef.h>

typedef int (*BuiltinFunc)(char **);

#define ALPHABET_SIZE 26

typedef struct trie_node_t {
  BuiltinFunc function;
  const char *name;
  struct trie_node_t *nodes[ALPHABET_SIZE];
} TrieNode;

void trie_insert(TrieNode *root, const char *str, BuiltinFunc function);

void trie_free(TrieNode *node);

void trie_init(TrieNode *root);

const TrieNode *find_node(const TrieNode *root, const char *str, size_t str_len);

#endif
