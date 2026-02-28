#include "alias.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter/lex.h"

// eventually this will get changed to use a hash map
struct alias_node {
  struct alias_node *p_next;
  char *key;
  token_t *tokens;
};

static struct alias_node *head = NULL;

void alias_set(const char *key, token_t *tokens) {
  if (head == NULL) {
    head = malloc(sizeof(struct alias_node));

    head->key = strdup(key);
    head->tokens = tokens;
    head->p_next = NULL;

    return;
  }

  struct alias_node *node = head;

  while (node != NULL) {
    if (strcmp(key, node->key) == 0) {
      free_tokens(&node->tokens);
      node->tokens = tokens;
      return;
    }

    if (node->p_next == NULL) {
      break;
    }

    node = node->p_next;
  }

  struct alias_node *new_node = malloc(sizeof(struct alias_node));

  new_node->key = strdup(key);
  new_node->tokens = tokens;
  new_node->p_next = NULL;

  node->p_next = new_node;
}

token_t *alias_get(const char *key) {
  struct alias_node *node = head;

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      return node->tokens;
    }

    node = node->p_next;
  }

  return NULL;
}

int alias_unset(const char *key) {
  struct alias_node *prev = NULL;
  struct alias_node *node = head;

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      free(node->key);

      free_tokens(&node->tokens);

      if (prev == NULL) {
        head = node->p_next;
      } else {
        prev->p_next = node->p_next;
      }

      free(node);
      return 0;
    }

    prev = node;
    node = node->p_next;
  }

  return 1;
}

void alias_print(void) {
  struct alias_node *node = head;

  while (node != NULL) {
    printf("%s\n", node->key);

    node = node->p_next;
  }
}
