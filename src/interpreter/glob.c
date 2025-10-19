#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../optional.h"
#include "lex.h"

typedef enum {
  TYPE_WILDCARD,
  TYPE_CHAR,
  TYPE_RANGE,
  TYPE_NEG_RANGE,
  TYPE_DIR_CHANGE,
  TYPE_END
} glob_type_t;

const char *const TYPE_NAMES[] = {"WILDCARD",  "CHAR",       "RANGE",
                                  "NEG_RANGE", "DIR_CHANGE", "END"};

typedef struct {
  glob_type_t type;
  union {
    char *range;
    char character;
  };
} glob_part_t;

typedef struct {
  bool relative_path;
  glob_part_t *parts;
} glob_t;

typedef enum { STATE_DEFAULT, STATE_RANGE } glob_state_t;

typedef OPTIONAL(glob_t) optional_glob;

optional_glob compile_glob(const uint8_t *const str) {
  bool relative_path = true;

  VECTOR(glob_part_t) glob_parts;
  VECTOR_INIT(glob_parts);

  glob_state_t state = STATE_DEFAULT;
  glob_part_t current;
  VECTOR(char) range = {0};
  size_t i = 0;

  if (str[0] == '/') {
    relative_path = false;
    i++;
  }

  for (; str[i] != '\0'; i++) {
    switch (state) {
      case STATE_DEFAULT:
        if (str[i] == '[') {
          current.type = TYPE_RANGE;

          VECTOR_INIT(range, 4);

          if (str[i] == '^' || str[i] == '!') {
            current.type = TYPE_NEG_RANGE;
            i++;
            continue;
          }

          state = STATE_RANGE;
          continue;
        }

        if (str[i] == '*') {
          VECTOR_PUSH(glob_parts, ((glob_part_t){.type = TYPE_WILDCARD}));
          continue;
        }

        if (str[i] == '/') {
          if (glob_parts.data[glob_parts.length - 1].type != TYPE_DIR_CHANGE) {
            VECTOR_PUSH(glob_parts, ((glob_part_t){.type = TYPE_DIR_CHANGE}));
          }
          continue;
        }

        VECTOR_PUSH(glob_parts,
                    ((glob_part_t){.type = TYPE_CHAR, .character = str[i]}));
        break;

      case STATE_RANGE:
        if (str[i] == ']') {
          if (range.length == 0) {
            goto error;
          }

          VECTOR_PUSH(range, '\0');
          current.range = range.data;
          VECTOR_PUSH(glob_parts, current);

          state = STATE_DEFAULT;
          continue;
        }

        VECTOR_PUSH(range, str[i]);
        break;
    }
  }

  if (state == STATE_RANGE) {
    goto error;
  }

  if (glob_parts.data[glob_parts.length - 1].type == TYPE_DIR_CHANGE) {
    glob_parts.data[glob_parts.length - 1].type = TYPE_END;
  } else {
    VECTOR_PUSH(glob_parts, ((glob_part_t){.type = TYPE_END}));
  }

  return (optional_glob){.has_value = true,
                         .value = (glob_t){.parts = glob_parts.data,
                                           .relative_path = relative_path}};

error:
  for (size_t j = 0; j < glob_parts.length; j++) {
    if (glob_parts.data[j].type == TYPE_RANGE ||
        glob_parts.data[j].type == TYPE_NEG_RANGE) {
      free(glob_parts.data[j].range);
    }
  }
  VECTOR_DESTROY(glob_parts);
  VECTOR_DESTROY(range);

  return (optional_glob){.has_value = false};
}

void print_glob(const glob_t glob) {
  printf("Relative Path: %s\n", glob.relative_path ? "True" : "False");
  for (size_t i = 0; glob.parts[i].type != TYPE_END; i++) {
    printf("  Part:\n    Type: %s\n", TYPE_NAMES[glob.parts[i].type]);

    if (glob.parts[i].type == TYPE_CHAR) {
      printf("    Char: %c\n", glob.parts[i].character);
    }

    if (glob.parts[i].type == TYPE_NEG_RANGE ||
        glob.parts[i].type == TYPE_RANGE) {
      printf("    Range: %s\n", glob.parts[i].range);
    }
  }
}

void free_glob(glob_t *glob) {
  for (size_t i = 0; glob->parts[i].type != TYPE_END; i++) {
    if (glob->parts[i].type == TYPE_RANGE ||
        glob->parts[i].type == TYPE_NEG_RANGE) {
      free(glob->parts[i].range);
    }
  }

  free(glob->parts);
}

bool match(const char *str, const glob_part_t *const parts) {
  for (size_t i = 0;; i++) {
    switch_here:
    switch (parts[i].type) {
      case TYPE_CHAR:
        if (parts[i].character != *str) {
          return false;
        }
        str++;
        break;

      case TYPE_RANGE: {
        bool found_match = false;

        for (size_t j = 0; parts[i].range[j] != '\0'; j++) {
          if (parts[i].range[j] == *str) {
            found_match = true;
            str++;
            break;
          }
        }

        if (found_match) {
          break;
        }

        return false;
      }

      case TYPE_NEG_RANGE: {
        bool found_match = false;

        for (size_t j = 0; parts[i].range[j] != '\0'; j++) {
          if (parts[i].range[j] != *str) {
            found_match = true;
            str++;
            break;
          }
        }

        if (found_match) {
          break;
        }

        return false;
      }

      case TYPE_DIR_CHANGE:
      case TYPE_END:
        break;
    }
  }

  return true;
}

int glob_str(const uint8_t *const str, tokens_t *tokens) {
  optional_glob optional = compile_glob(str);

  if (!optional.has_value) {
    return 0;
  }

  glob_t glob = optional.value;

  VECTOR(char *) stack;
  VECTOR_INIT(stack, 1);

  DIR *dir;
  if (glob.relative_path) {
    dir = opendir(".");
  } else {
    dir = opendir("/");
  }

  if (dir == NULL) {
    perror("opendir");
    return 0;
  }

  int matches = 0;

  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    if (match(ent->d_name, glob.parts)) {
      matches++;
      VECTOR_PUSH(*tokens,
                  ((token_t){.type = STRING, .data = strdup(ent->d_name)}));
    }
  }

  return matches;
}
