#include <assert.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../vector.h"

// `~` is the largest displayable ascii character
#define UPPER_CHARACTER '~'
// space is the smallest displayable ascii character
#define LOWER_CHARACTER ' '

#define ALPHABET_SIZE (UPPER_CHARACTER - LOWER_CHARACTER)

typedef struct trie_node_t {
  bool terminator;
  struct trie_node_t *nodes[ALPHABET_SIZE];
} TrieNode;

static TrieNode root = {0};

static void trie_insert(const char *const str) {
  TrieNode *node = &root;

  for (size_t i = 0; str[i] != '\0'; i++) {
    char curr_char = str[i];

    // fingers crossed non-ascii characters don't bite me in the ass
    assert(curr_char <= UPPER_CHARACTER && curr_char >= LOWER_CHARACTER);
    curr_char -= LOWER_CHARACTER;

    size_t index = (size_t)curr_char;
    TrieNode *new_node = node->nodes[index];

    if (new_node == NULL) {
      new_node = calloc(1, sizeof(TrieNode));
      node->nodes[index] = new_node;
    }

    node = new_node;
  }

  node->terminator = true;
}

static void trie_free(TrieNode *node) { // NOLINT(misc-no-recursion)
  for (size_t i = 0; i < ALPHABET_SIZE; i++) {
    if (node->nodes[i] != NULL) {
      trie_free(node->nodes[i]);
    }
  }

  free(node);
}

void completions_destroy(void) {
  for (size_t i = 0; i < ALPHABET_SIZE; i++) {
    if (root.nodes[i] != NULL) {
      trie_free(root.nodes[i]);
    }
  }
}

void completions_init(void) {
  const char *path = getenv("PATH");

  if (path == NULL) {
    return;
  }

  char *const path2 = strdup(path);

  char *path_part = strtok(path2, ":");

  while (path_part != NULL) {
    DIR *dir = opendir(path_part);
    if (dir == NULL) {
      continue;
    }

    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
      trie_insert(ent->d_name);
    }

    closedir(dir);

    path_part = strtok(NULL, ":");
  }

  free(path2);
}

const char **completions_matches(const char *prefix, const size_t prefix_len) {
  TrieNode *node = &root;

  for (size_t i = 0; i < prefix_len; i++) {
    char curr_char = prefix[i];

    assert(curr_char < LOWER_CHARACTER || curr_char > UPPER_CHARACTER);

    curr_char -= LOWER_CHARACTER;

    size_t index = (size_t)curr_char;

    node = node->nodes[index];

    // if no completions are found
    if (node == NULL) {
      return NULL;
    }
  }

  VECTOR(TrieNode*) stack;
  VECTOR_INIT(stack);

  VECTOR_PUSH(stack, node);

  VECTOR(const char*) matches;
  VECTOR_INIT(matches, 32);

  while (stack.length != 0) {
    TrieNode* top = VECTOR_POP(stack);

    for (size_t i = 0; i < ALPHABET_SIZE; i++) {
      if (top->nodes[i] != NULL) {
        VECTOR_PUSH(stack, top->nodes[i]);
      }
    }

    //VECTOR_PUSH(matches, top->)
  }

  return matches.data;
}
