#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct env_node {
  struct env_node *p_next;
  // NAME=VALUE, same format as environ
  char *string;
};

static struct env_node *head = NULL;

extern char **environ;

static char **new_environ = NULL;

static inline bool
key_compare(const char *const env_string, const char *const key) {
  size_t i = 0;
  while (1) {
    if (env_string[i] == '\0') {
      return false;
    }

    if (key[i] == '\0') {
      return (env_string[i] == '=');
    }
  }
}

int env_set(const char *const key, const char *const value) {
  // make sure the key doesn't contain '=' (posix forbids this)
  size_t key_len;
  for (key_len = 0; key[key_len] != '\0'; key_len++) {
    if (key[key_len] == '=') {
      return 1;
    }
  }

  if (key_len == 0) {
    return 1;
  }

  size_t val_len = strlen(value);

  // generate the environment variable string ("KEY=VALUE"), same as environ
  char *env_string = malloc(strlen(key) + strlen(value) + 2);
  memcpy(env_string, key, key_len);
  env_string[key_len] = '=';
  memcpy(env_string + key_len + 1, value, val_len + 1);

  // after setting an env, new_environ will be out of date
  new_environ = NULL;

  if (head == NULL) {
    head = malloc(sizeof(struct env_node));

    head->string = env_string;
    head->p_next = NULL;

    return 0;
  }

  struct env_node *node = head;

  while (node->p_next != NULL) {
    if (key_compare(node->string, key)) {
      free(node->string);
      node->string = env_string;
      return 0;
    }

    node = node->p_next;
  }

  struct env_node *new_node = malloc(sizeof(struct env_node));

  new_node->string = env_string;
  new_node->p_next = NULL;

  node->p_next = new_node;
  return 0;
}

int env_put(const char *const env_string) {
  if (env_string[0] == '=' || env_string[0] == '\0') {
    return 1;
  }
  
  size_t str_len = strlen(env_string);

  char *new_str = malloc(str_len);
  memcpy(new_str, env_string, str_len);

  // after setting an env, new_environ will be out of date
  new_environ = NULL;

  return 0;
}

const char *env_get(const char *const key) {
  struct env_node *node = head;
  size_t key_len = strlen(key);

  while (node != NULL) {
    if (key_compare(key, node->string)) {
      return node->string + key_len + 1;
    }

    node = node->p_next;
  }

  return NULL;
}

int env_unset(const char *const key) {
  struct env_node *prev = NULL;
  struct env_node *node = head;

  while (node != NULL) {
    if (key_compare(node->string, key)) {
      free(node->string);

      if (prev == NULL) {
        head = node->p_next;
      } else {
        prev->p_next = node->p_next;
      }

      free(node);

      // after removing a variable, new_environ will be out of date
      new_environ = NULL;

      return 0;
    }

    prev = node;
    node = node->p_next;
  }

  return 1;
}

void env_init(void) {
  char **e = environ;

  while (*e != NULL) {
    if (env_put(*e)) {
      fprintf(stderr, "rash: environment variable ‘%s’ is malformed.\n", *e);
    }

    e++;
  }
}

char** env_get_environ(void) {
  if (new_environ) {
    return new_environ;
  }

  size_t environ_len = 1;
  struct env_node* node = head;

  while (node) {
    environ_len++;
    node = node->p_next;
  }

  new_environ = malloc(sizeof(char*) * environ_len);

  node = head;
  size_t i = 0;

  while (node) {
    new_environ[i] = node->string;
    node = node->p_next;
    i++;
  }

  new_environ[i] = NULL;

  return new_environ;
}

void env_print(void) {
  struct env_node *node = head;

  while (node != NULL) {
    printf("%s\n", node->string);
  }

  return;
}
