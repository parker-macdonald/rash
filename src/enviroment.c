#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct env {
  char *env_var;
  struct env *p_next;
} env_t;

static env_t *env_start;
static env_t *env_end;
static size_t length;

void env_init(void) {
  env_start = NULL;
  env_end = NULL;

  length = 0;
}

env_t *env_get_node_from_name(const char *const env_str) {
  env_t *current = env_start;
  size_t env_equals;

  for (env_equals = 0; env_str[env_equals] != '='; env_equals++)
    ;

  while (current != NULL) {
    size_t equals;

    for (equals = 0; current->env_var[equals] != '='; equals++)
      ;

    if (equals != env_equals) {
      current = current->p_next;
      continue;
    }

    if (strncmp(current->env_var, env_str, equals) == 0) {
      return current;
    }

    current = current->p_next;
  }

  return NULL;
}

void env_add(const char *const env_str) {
  char *env_cpy = strdup(env_str);

  env_t *node = env_get_node_from_name(env_cpy);

  if (node == NULL) {
    node = malloc(sizeof(env_t));
    node->p_next = NULL;

    if (env_start == NULL) {
      env_start = node;
    } else {
      env_end->p_next = node;
    }

    env_end = node;
    length++;
  } else {
    free(node->env_var);
  }

  node->env_var = env_cpy;
}

void env_remove(const char *const env_str) {
  env_t *current = env_start;
  env_t *prev = NULL;

  while (current != NULL) {
    size_t equals;

    for (equals = 0; current->env_var[equals] != '='; equals++)
      ;

    if (strncmp(current->env_var, env_str, equals) == 0) {
      if (prev != NULL) {
        prev->p_next = current->p_next;
      } else {
        env_start = current->p_next;
      }

      free(current->env_var);
      free(current);

      length--;

      return;
    }

    prev = current;
    current = current->p_next;
  }
}

char *env_get(const char *const env_name) {
  env_t *current = env_start;

  while (current != NULL) {
    size_t equals;

    for (equals = 0; current->env_var[equals] != '='; equals++)
      ;

    if (strncmp(current->env_var, env_name, equals) == 0) {
      return current->env_var + equals + 1;
    }

    current = current->p_next;
  }

  return "";
}

char **env_get_array(void) {
  char **array = malloc(sizeof(char *) * (length + 1));
  env_t *current = env_start;

  for (size_t i = 0; current != NULL; i++) {
    array[i] = current->env_var;

    current = current->p_next;
  }

  array[length] = NULL;

  return array;
}

void env_destroy(void) {
  env_t *current = env_start;
  env_t *previous = NULL;

  while (current != NULL) {
    free(current->env_var);
    free(previous);

    previous = current;
    current = current->p_next;
  }

  free(previous);
}