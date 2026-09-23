#include "builtins/builtins.h"

#include <stdlib.h>
#include <string.h>

#include "builtins/trie.h"
#include "lib/buffer.h"
#include "lib/vector.h"

void builtins_init(Builtins *self) {
  TrieNode *root = malloc(sizeof(TrieNode));

  trie_init(root);

  self->root = root;
}

void builtins_destroy(Builtins *self) {
  trie_free(self->root);
}

BuiltinFunc find_builtin(const Builtins *self, const char *str) {
  size_t str_len = strlen(str);
  const TrieNode *node = find_node(self->root, str, str_len);

  if (node == NULL) {
    return NULL;
  }

  return node->function;
}

void find_matching_builtins(const Builtins *self, const Buffer *prefix, BufferList *list) {
  const TrieNode *node = find_node(self->root, prefix->char_ptr, prefix->length);

  if (node == NULL) {
    return;
  }

  VECTOR(TrieNode *) stack;
  VECTOR_INIT(stack);

  do {
    if (node->function != NULL) {
      Buffer name = buffer_from_cstr(node->name);
      buffer_append(&name, ' ');

      VECTOR_PUSH(*list, name);
      continue;
    }
    for (size_t i = 0; i < ALPHABET_SIZE; i++) {
      if (node->nodes[i] != NULL) {
        VECTOR_PUSH(stack, node->nodes[i]);
      }
    }

  } while ((node = VECTOR_POP(stack, NULL)));

  VECTOR_DESTROY(stack);
}