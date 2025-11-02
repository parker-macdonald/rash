#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../vector.h"
#include "evaluate.h"

struct queue_node {
  struct queue_node *p_next;
  char *path;
  // path_len does not include the null terminator
  size_t path_len;
};

// modified Krauss's wildcard matching algorithm
static bool match(const char *str, const char *pattern) {
  if (str[0] == '.' && pattern[0] != '.') {
    return false;
  }

  const char *after_last_wild = NULL;
  // Location after last "*", if we've encountered one
  char t, w;
  // Walk the text strings one character at a time.
  while (1) {
    t = *str;
    w = *pattern;
    // How do you match a unique text string?
    if (!t || t == '/') {
      // Easy: unique up on it!
      if (!w || w == '/') {
        return true; // "x" matches "x"
      } else if (w == '\033') {
        pattern++;
        continue; // "x*" matches "x" or "xy"
      }
      return false; // "x" doesn't match "xy"
    } else {
      // How do you match a tame text string?
      if (t != w) {
        // The tame way: unique up on it!
        if (w == '\033') {
          after_last_wild = ++pattern;
          continue; // "*y" matches "xy"
        } else if (after_last_wild) {
          pattern = after_last_wild;
          w = *pattern;

          if (!w || w == '/') {
            return true; // "*" matches "x"
          } else if (t == w) {
            pattern++;
          }
          str++;
          continue; // "*sip*" matches "mississippi"
        } else {
          return false; // "x" doesn't match "y"
        }
      }
    }
    str++;
    pattern++;
  }
  return true;
}

int glob(argv_t *argv, const char *pattern) {
  struct queue_node *head = malloc(sizeof(struct queue_node));
  struct queue_node *tail = head;
  size_t queue_length = 1;
  int args_added = 0;

  head->p_next = NULL;
  head->path_len = 1;
  head->path = malloc(sizeof(char) * 2);
  head->path[1] = '\0';

  if (pattern[0] == '/') {
    pattern++;
    head->path[0] = '/';
  } else {
    head->path[0] = '.';
  }

  while (queue_length > 0) {
    DIR *dir = opendir(head->path);
    struct dirent *ent;

    if (dir == NULL) {
      if (errno == ENOTDIR) {
        continue;
      }

      assert(0);
    }

    while ((ent = readdir(dir)) != NULL) {
      if (match(ent->d_name, pattern)) {
        const size_t ent_len = strlen(ent->d_name);
        const size_t path_len = head->path_len;

        const size_t new_path_len = path_len + ent_len + 1;
        char *new_path = malloc(new_path_len + 1);
        memcpy(new_path, head->path, path_len);
        new_path[path_len] = '/';
        memcpy(new_path + path_len + 1, ent->d_name, ent_len);
        new_path[new_path_len] = '\0';

        struct queue_node *node = malloc(sizeof(struct queue_node));
        node->path = new_path;
        node->path_len = new_path_len;
        node->p_next = NULL;

        tail->p_next = node;
        tail = node;
        queue_length++;
      }
    }

    closedir(dir);

    struct queue_node *temp = head->p_next;
    free(head->path);
    free(head);
    head = temp;
    queue_length--;

    for (;; pattern++) {
      if (*pattern == '/') {
        pattern++;
        if (*pattern == '\0') {
          goto final;
        }
        break;
      }

      if (*pattern == '\0') {
        goto final;
      }
    }
  }

final:
  while (head != NULL) {
    VECTOR_PUSH(*argv, head->path);

    struct queue_node *temp = head->p_next;
    free(head);
    head = temp;
  }

  return args_added;
}
