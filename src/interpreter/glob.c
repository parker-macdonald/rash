#include "glob.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "lib/error.h"
#include "lib/vec_types.h"
#include "lib/vector.h"

struct queue_node {
  struct queue_node *p_next;
  char *path;
  // path_len does not include the null terminator
  size_t path_len;
  size_t pattern_index;
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
      }
      if (w == '\033') {
        pattern++;
        continue; // "x*" matches "x" or "xy"
      }
      return false; // "x" doesn't match "xy"
    }
    // How do you match a tame text string?
    if (t != w) {
      // The tame way: unique up on it!
      if (w == '\033') {
        after_last_wild = ++pattern;
        continue; // "*y" matches "xy"
      }
      if (after_last_wild) {
        pattern = after_last_wild;
        w = *pattern;

        if (!w || w == '/') {
          return true; // "*" matches "x"
        }
        if (t == w) {
          pattern++;
        }
        str++;
        continue; // "*sip*" matches "mississippi"
      }
      return false; // "x" doesn't match "y"
    }
    str++;
    pattern++;
  }
  return true;
}

int glob(strings_t *vec, const char pattern[]) {
  struct queue_node *head = malloc(sizeof(struct queue_node));
  struct queue_node *tail = head;
  int args_added = 0;

  head->p_next = NULL;

  size_t last_slash = (size_t)(-1);
  for (size_t i = 0;; i++) {
    if (pattern[i] == '\033') {
      break;
    }
    if (pattern[i] == '/') {
      last_slash = i;
    }
  }

  if (last_slash == (size_t)(-1)) {
    head->path = malloc(2);
    head->path[0] = '.';
    head->path[1] = '\0';
    head->path_len = 0;
    head->pattern_index = 0;
  } else {
    head->path = malloc(last_slash + 2);
    memcpy(head->path, pattern, last_slash + 1);
    head->path[last_slash + 1] = '\0';
    head->path_len = last_slash + 1;
    head->pattern_index = last_slash + 1;
  }

  while (head != NULL) {
    DIR *dir = opendir(head->path);
    struct dirent *ent;

    if (dir == NULL) {
      if (errno == ENOTDIR || errno == EACCES) {
        struct queue_node *temp = head->p_next;
        free(head->path);
        free(head);
        head = temp;
        continue;
      }

      error_f("glob: opendir: %s", strerror(errno));
      struct queue_node *node = head;

      while (node != NULL) {
        struct queue_node *temp = node->p_next;
        free(node->path);
        free(node);
        node = temp;
      }

      return -1;
    }

    bool end = false;
    bool found_wildcard = false;
    size_t new_pattern_index = head->pattern_index;
    for (;; new_pattern_index++) {
      if (pattern[new_pattern_index] == '\033') {
        found_wildcard = true;
      }
      if (found_wildcard && pattern[new_pattern_index] == '/') {
        new_pattern_index++;
        if (pattern[new_pattern_index] == '\0') {
          end = true;
        }
        break;
      }

      if (pattern[new_pattern_index] == '\0') {
        end = true;
        break;
      }
    }

    while ((ent = readdir(dir)) != NULL) {
      if (match(ent->d_name, pattern + head->pattern_index)) {
        const size_t ent_len = strlen(ent->d_name);

        const size_t new_path_len = head->path_len + ent_len + 1;
        char *new_path = malloc(new_path_len + 1);

        // sprintf(new_path, "%s%s/", head->path, ent->d_name);
        memcpy(new_path, head->path, head->path_len);
        memcpy(new_path + head->path_len, ent->d_name, ent_len);
        new_path[new_path_len - 1] = '/';
        new_path[new_path_len] = '\0';

        if (end) {
          new_path[new_path_len - 1] = '\0';
          VECTOR_PUSH(*vec, new_path);
          args_added++;
          continue;
        }

        struct queue_node *node = malloc(sizeof(struct queue_node));
        node->path = new_path;
        node->path_len = new_path_len;
        node->p_next = NULL;
        node->pattern_index = new_pattern_index;

        tail->p_next = node;
        tail = node;
      }
    }

    closedir(dir);

    struct queue_node *temp = head->p_next;
    free(head->path);
    free(head);
    head = temp;
  }

  return args_added;
}
