#include "../../line_reader.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 10

int builtin_history(char **const argv) {
  line_node_t *node = root_line_node;

  if (argv[1] != NULL) {
    if (strcmp(argv[1], "-c") == 0) {
      clear_history();
      return EXIT_SUCCESS;
    }

    char *endptr;
    errno = 0;
    const long num = strtol(argv[1], &endptr, BASE);
    if (errno != 0 || *endptr != '\0' || num < 0) {
      fprintf(stderr, "history: %s: positive number expected\n", argv[1]);
      return EXIT_FAILURE;
    }

    unsigned int count = (unsigned int)num;

    if (count == 0) {
      return EXIT_SUCCESS;
    }

    // traverse to the last line
    unsigned int length;
    for (length = 1;; length++) {
      if (node->p_next == NULL) {
        break;
      }

      node = node->p_next;
    }

    if (count < length) {
      // backtrack the number of lines to print
      for (unsigned int i = 1; i < count; i++) {
        node = node->p_prev;
      }
    } else {
      node = root_line_node;
      count = length;
    }

    for (unsigned int i = count - 1; i >= 0 && node != NULL; i--) {
      printf("%5u  ", length - i);
      PRINT_LINE(node->line);
      fputs("\n", stdout);
      node = node->p_next;
    }

    return EXIT_SUCCESS;
  }

  for (unsigned int i = 1; node != NULL; i++) {
    printf("%5u  ", i);
    PRINT_LINE(node->line);
    fputs("\n", stdout);
    node = node->p_next;
  }

  return EXIT_SUCCESS;
}
