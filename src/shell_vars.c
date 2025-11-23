#include <stdlib.h>
#include <string.h>

// eventually this will get changed to use a hash map
struct var_node {
  struct var_node *p_next;
  char *key;
  char *value;
};

static struct var_node *head = NULL;

void set_var(const char *const key, const char *const value) {
  if (head == NULL) {
    head = malloc(sizeof(struct var_node));

    head->key = strdup(key);
    head->value = strdup(value);
    head->p_next = NULL;

    return;
  }

  struct var_node *node = head;

  while (node->p_next != NULL) {
    if (strcmp(key, node->key) == 0) {
      free(node->value);
      node->value = strdup(value);
      return;
    }

    node = node->p_next;
  }

  struct var_node *new_node = malloc(sizeof(struct var_node));

  new_node->key = strdup(key);
  new_node->value = strdup(value);
  new_node->p_next = NULL;

  node->p_next = new_node;
}

const char *get_var(const char *const key) {
  struct var_node *node = head;

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      return node->value;
    }

    node = node->p_next;
  }

  return NULL;
}
