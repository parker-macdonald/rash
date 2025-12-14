#include "shell_vars.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// eventually this will get changed to use a hash map
struct var_node {
  struct var_node *p_next;
  char *key;
  char *value;
};

static struct var_node *head = NULL;

void var_set(const char *const key, const char *const value) {
  if (head == NULL) {
    head = malloc(sizeof(struct var_node));

    head->key = strdup(key);
    head->value = strdup(value);
    head->p_next = NULL;

    return;
  }

  struct var_node *node = head;

  while (node != NULL) {
    if (strcmp(key, node->key) == 0) {
      free(node->value);
      node->value = strdup(value);
      return;
    }

    if (node->p_next == NULL) {
      break;
    }

    node = node->p_next;
  }

  struct var_node *new_node = malloc(sizeof(struct var_node));

  new_node->key = strdup(key);
  new_node->value = strdup(value);
  new_node->p_next = NULL;

  node->p_next = new_node;
}

const char *var_get(const char *const key) {
  struct var_node *node = head;

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      return node->value;
    }

    node = node->p_next;
  }

  return NULL;
}

int var_unset(const char *const key) {
  struct var_node *prev = NULL;
  struct var_node *node = head;

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      free(node->key);
      free(node->value);

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

void var_print(void) {
  struct var_node *node = head;

  while (node != NULL) {
    printf("{%s}:\t\"%s\"\n", node->key, node->value);

    node = node->p_next;
  }
}
