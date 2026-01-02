#include "./find_builtin.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../vec_types.h"
#include "../vector.h"
#include "./builtins.h"

#define ALPHABET_SIZE 26

typedef struct trie_node_t {
  builtin_t function;
  const char *name;
  struct trie_node_t *nodes[ALPHABET_SIZE];
} TrieNode;

static TrieNode root = {0};

static void trie_insert(const char *const str, const builtin_t function) {
  TrieNode *node = &root;

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
static void trie_free(TrieNode *node) { // NOLINT(misc-no-recursion)
  for (size_t i = 0; i < ALPHABET_SIZE; i++) {
    if (node->nodes[i] != NULL) {
      trie_free(node->nodes[i]);
    }
  }

  free(node);
}

void trie_destroy(void) {
  for (size_t i = 0; i < ALPHABET_SIZE; i++) {
    if (root.nodes[i] != NULL) {
      trie_free(root.nodes[i]);
    }
  }
}

void trie_init(void) {
  trie_insert("cd", &builtin_cd);
  trie_insert("help", &builtin_help);
  trie_insert("exit", &builtin_exit);
  trie_insert("export", &builtin_export);
  trie_insert("history", &builtin_history);
  trie_insert("true", &builtin_true);
  trie_insert("false", &builtin_false);
  trie_insert("pwd", &builtin_pwd);
  trie_insert("fg", &builtin_fg);
  trie_insert("bg", &builtin_bg);
  trie_insert("jobs", &builtin_jobs);
  trie_insert("version", &builtin_version);
  trie_insert("setvar", &builtin_setvar);
  trie_insert("unsetvar", &builtin_unsetvar);
  trie_insert("source", &builtin_source);
  trie_insert("which", &builtin_which);
  trie_insert("var", &builtin_var);
  trie_insert("env", &builtin_env);
  trie_insert("setenv", &builtin_setenv);
  trie_insert("unsetenv", &builtin_unsetenv);
  trie_insert("exec", &builtin_exec);
}

static TrieNode *find_node(const char *str, size_t str_len) {
  TrieNode *node = &root;
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

builtin_t find_builtin(const char *const str) {
  size_t str_len = strlen(str);
  TrieNode *node = find_node(str, str_len);

  if (node == NULL) {
    return NULL;
  }

  return node->function;
}

void find_matching_builtins(
    const char *prefix, size_t prefix_len, strings_t *vec
) {
  TrieNode *node = find_node(prefix, prefix_len);

  if (node == NULL) {
    return;
  }

  VECTOR(TrieNode *) stack;
  VECTOR_INIT(stack);

  do {
    if (node->function != NULL) {
      size_t name_len = strlen(node->name);
      char *name = malloc(name_len + 2);
      memcpy(name, node->name, name_len);
      name[name_len] = ' ';
      name[name_len + 1] = '\0';

      VECTOR_PUSH(*vec, name);
      continue;
    }
    for (size_t i = 0; i < ALPHABET_SIZE; i++) {
      if (node->nodes[i] != NULL) {
        VECTOR_PUSH(stack, node->nodes[i]);
      }
    }

  } while ((node = VECTOR_POP(stack)));
}
