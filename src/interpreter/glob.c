#include "glob.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../vector.h"
#include "evaluate.h"

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
  int args_added = 0;

  head->p_next = NULL;
  head->path = malloc(sizeof(char) * 2);
  head->path_len = 1;
  head->pattern_index = 0;

  head->path[1] = '\0';
  if (pattern[0] == '/') {
    pattern++;
    head->path[0] = '/';
  } else {
    head->path[0] = '.';
  }

  while (head != NULL) {
    DIR *dir = opendir(head->path);
    struct dirent *ent;

    if (dir == NULL) {
      if (errno == ENOTDIR) {
        struct queue_node *temp = head->p_next;
        free(head->path);
        free(head);
        head = temp;
        continue;
      }

      if (errno == EACCES) {
        struct queue_node *temp = head->p_next;
        free(head->path);
        free(head);
        head = temp;
        continue;
      }

      perror("glob: opendir");
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
    size_t new_pattern_index = head->pattern_index;
    for (;; new_pattern_index++) {
      if (pattern[new_pattern_index] == '/') {
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
        const size_t path_len = head->path_len;

        const size_t new_path_len = path_len + ent_len + 1;
        char *new_path = malloc(new_path_len + 1);
        memcpy(new_path, head->path, path_len);
        new_path[path_len] = '/';
        memcpy(new_path + path_len + 1, ent->d_name, ent_len);
        new_path[new_path_len] = '\0';

        if (end) {
          VECTOR_PUSH(*argv, new_path);
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
