#include "alias.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// eventually this will get changed to use a hash map
struct alias_node {
  struct alias_node *p_next;
  char *key;
  char **value;
  size_t length;
};

static struct alias_node *head = NULL;

void alias_set(const char *key, char *value[], size_t length) {
  char **value_clone = malloc(sizeof(char *) * length);

  for (size_t i = 0; i < length; i++) {
    value_clone[i] = strdup(value[i]);
  }

  if (head == NULL) {
    head = malloc(sizeof(struct alias_node));

    head->key = strdup(key);
    head->value = value_clone;
    head->length = length;
    head->p_next = NULL;

    return;
  }

  struct alias_node *node = head;

  while (node != NULL) {
    if (strcmp(key, node->key) == 0) {
      free(node->value);
      head->length = length;
      node->value = value_clone;
      return;
    }

    if (node->p_next == NULL) {
      break;
    }

    node = node->p_next;
  }

  struct alias_node *new_node = malloc(sizeof(struct alias_node));

  new_node->key = strdup(key);
  new_node->value = value_clone;
  new_node->length = length;
  new_node->p_next = NULL;

  node->p_next = new_node;
}

alias_or_none alias_get(const char *key) {
  struct alias_node *node = head;

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      return (alias_or_none){
          .has_value = true,
          .value = (struct alias_t) {
            .length = node->length,
            .value = node->value
          }
      };
    }

    node = node->p_next;
  }

  return (alias_or_none){.has_value = false};
}

int alias_unset(const char *key) {
  struct alias_node *prev = NULL;
  struct alias_node *node = head;

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      free(node->key);

      for (size_t i = 0; i < node->length; i++) {
        free(node->value[i]);
      }
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

void alias_print(void) {
  struct alias_node *node = head;

  while (node != NULL) {
    printf("%s:", node->key);

    for (size_t i = 0; i < node->length; i++) {
      printf(" %s", node->value[i]);
    }

    putchar('\n');

    node = node->p_next;
  }
}
