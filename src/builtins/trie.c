#include "trie.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtin_funcs.h"

void trie_insert(TrieNode *root, const char *const str, const BuiltinFunc function) {
  TrieNode *node = root;

  for (size_t i = 0; str[i] != '\0'; i++) {
    char curr_char = str[i];

    // all builtins are just letters i believe
    assert(isalpha((int)curr_char));
    curr_char -= 'a';
    size_t index = (size_t)curr_char;
    TrieNode *new_node = node->nodes[index];

    if (new_node == NULL) {
      new_node = calloc(1, sizeof(TrieNode));
      node->nodes[index] = new_node;
    }

    node = new_node;
  }

  node->name = str;
  node->function = function;
}

// the trie should never get too big, so i don't see recursion as too big of a
// problem here.
void trie_free(TrieNode *node) { // NOLINT(misc-no-recursion)
  for (size_t i = 0; i < ALPHABET_SIZE; i++) {
    if (node->nodes[i] != NULL) {
      trie_free(node->nodes[i]);
    }
  }

  free(node);
}

void trie_init(TrieNode *root) {
  trie_insert(root, "cd", &builtin_cd);
  trie_insert(root, "help", &builtin_help);
  trie_insert(root, "exit", &builtin_exit);
  trie_insert(root, "export", &builtin_export);
  trie_insert(root, "history", &builtin_history);
  trie_insert(root, "true", &builtin_true);
  trie_insert(root, "false", &builtin_false);
  trie_insert(root, "pwd", &builtin_pwd);
  trie_insert(root, "fg", &builtin_fg);
  trie_insert(root, "bg", &builtin_bg);
  trie_insert(root, "jobs", &builtin_jobs);
  trie_insert(root, "version", &builtin_version);
  trie_insert(root, "setvar", &builtin_setvar);
  trie_insert(root, "unsetvar", &builtin_unsetvar);
  trie_insert(root, "source", &builtin_source);
  trie_insert(root, "which", &builtin_which);
  trie_insert(root, "var", &builtin_var);
  trie_insert(root, "env", &builtin_env);
  trie_insert(root, "setenv", &builtin_setenv);
  trie_insert(root, "unsetenv", &builtin_unsetenv);
  trie_insert(root, "exec", &builtin_exec);
  trie_insert(root, "eval", &builtin_eval);
  trie_insert(root, "time", &builtin_time);
  trie_insert(root, "mkdir", &builtin_mkdir);
}

const TrieNode *find_node(const TrieNode *root, const char *str, size_t str_len) {
  const TrieNode *node = root;
  for (size_t i = 0; i < str_len; i++) {
    char curr_char = str[i];

    if (curr_char < 'a' || curr_char > 'z') {
      return NULL;
    }

    curr_char -= 'a';

    size_t index = (size_t)curr_char;
    node = node->nodes[index];

    if (node == NULL) {
      return NULL;
    }
  }

  return node;
}
