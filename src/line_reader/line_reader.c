#include "line_reader.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auto_complete.h"
#include "lib/ansi.h"
#include "lib/sort.h"
#include "lib/utf_8.h"
#include "lib/vec_types.h"
#include "lib/vector.h"
#include "modify_line.h"
#include "prompt.h"
#include "shell_vars.h"
#include "utils.h"

#ifdef static_assert
static_assert(
    sizeof(char) == sizeof(uint8_t),
    "char is not one byte in size, god save you..."
);
#endif

typedef struct line_node {
  struct line_node *p_next;
  struct line_node *p_prev;
  // line that can be mutated by using the up and down arrows
  buf_t mut_line;
  // line thats used for displaying history
  uint8_t *const_line;
  size_t const_line_len;
} line_node_t;

static line_node_t *root_line_node = NULL;
static line_node_t *last_line_node = NULL;

void clear_history(void) {
  line_node_t *node = root_line_node;

  while (node != NULL) {
    line_node_t *next_node = node->p_next;

    free(node->const_line);
    VECTOR_DESTROY(node->mut_line);
    free(node);

    node = next_node;
  }
  root_line_node = NULL;
  last_line_node = NULL;
}

void line_reader_destroy(void) {
  clear_history();

  root_line_node = NULL;
  last_line_node = NULL;
}

void print_history(int count) {
  assert(count >= -1);

  if (count == 0) {
    return;
  }

  line_node_t *node = root_line_node;

  if (count == -1) {
    for (unsigned int i = 1; node != NULL; i++) {
      printf(
          "%5u  %.*s\n", i, (int)node->const_line_len, (char *)node->const_line
      );
      node = node->p_next;
    }

    return;
  }

  // traverse to the last line
  int length;
  for (length = 1;; length++) {
    if (node->p_next == NULL) {
      break;
    }

    node = node->p_next;
  }

  if (count < length) {
    // backtrack the number of lines to print
    for (int i = 1; i < count; i++) {
      node = node->p_prev;
    }
  } else {
    node = root_line_node;
    count = length;
  }

  for (unsigned int i = (unsigned int)count - 1; node != NULL; i--) {
    printf(
        "%5u  %.*s\n", i, (int)node->const_line_len, (char *)node->const_line
    );
    node = node->p_next;
  }
}

#define CURSOR_RIGHT                                                           \
  do {                                                                         \
    displayed_cursor_pos++;                                                    \
    if (displayed_cursor_pos % width == 0) {                                   \
      (void)fputs("\r\033[B", stdout);                                         \
    } else {                                                                   \
      (void)fputs(ANSI_CURSOR_RIGHT, stdout);                                  \
    }                                                                          \
  } while (0)

#define CURSOR_LEFT                                                            \
  do {                                                                         \
    displayed_cursor_pos--;                                                    \
    if ((displayed_cursor_pos + 1) % width == 0) {                             \
      (void)fputs("\033[999C\033[A", stdout);                                  \
    } else {                                                                   \
      (void)fputs(ANSI_CURSOR_LEFT, stdout);                                   \
    }                                                                          \
  } while (0)

#define CURSOR_RIGHT_N(n)                                                      \
  do {                                                                         \
    size_t moves_down = ((displayed_cursor_pos + (n)) / width) -               \
                        (displayed_cursor_pos / width);                        \
    if (moves_down > 0) {                                                      \
      printf("\033[%zuB", moves_down);                                         \
    }                                                                          \
    size_t moves_right = displayed_cursor_pos % width;                         \
    if (moves_right) {                                                         \
      printf("\r\033[%zuC", moves_right);                                      \
    }                                                                          \
  } while (0)

#define CURSOR_LEFT_N(n)                                                       \
  do {                                                                         \
    size_t moves_up = (displayed_cursor_pos / width) -                         \
                      ((displayed_cursor_pos - (n)) / width);                  \
    if (moves_up > 0) {                                                        \
      printf("\033[%zuA", moves_up);                                           \
    }                                                                          \
    size_t moves_left = displayed_cursor_pos % width;                          \
    if (moves_left) {                                                          \
      printf("\r\033[%zuC", moves_left);                                       \
    }                                                                          \
  } while (0)

#define DRAW_LINE(line)                                                        \
  printf("\r\033[0J%s%.*s", prompt, (int)(line).length, (char *)(line).data)

const uint8_t *readline(void *_) {
  (void)_;

  const char *prompt_var = var_get("PS1");
  char *prompt;
  unsigned int prompt_length;
  bool show_matches = false;

  if (prompt_var == NULL) {
    prompt = "$ ";
    prompt_length = 2;
  } else {
    prompt_length = get_prompt(&prompt, prompt_var);
  }

  printf("\r%s", prompt);
  (void)fflush(stdout);

  unsigned short width = get_terminal_width();
  size_t characters_printed = prompt_length;

  line_node_t *node = NULL;

  buf_t line;
  VECTOR_INIT(line);

  size_t cursor_pos = 0;
  size_t displayed_cursor_pos = characters_printed;

  uint8_t curr_byte;
  for (;;) {
    int ch = getch();

    // this is sent by the sigint handler to let the line reader know the user
    // pressed ctrl-c to trigger a SIGINT.
    if (ch == SIGINT_ON_READ) {
      printf("\n%s", prompt);
      (void)fflush(stdout);

      characters_printed = prompt_length;
      line.length = 0;
      cursor_pos = 0;
      continue;
    }

    curr_byte = (uint8_t)ch;

    if (curr_byte == ASCII_END_OF_TRANSMISSION) {
      printf("\n");
      if (prompt_var != NULL) {
        free(prompt);
      }
      VECTOR_DESTROY(line);
      line_reader_destroy();
      return NULL;
    }

    // the current line for editing
    buf_t *current_line = node == NULL ? &line : &node->mut_line;

    if (curr_byte == '\n' || curr_byte == '\r') {
      if (current_line->length != 0) {
        break;
      }

      printf("\n%s", prompt);
      (void)fflush(stdout);
      continue;
    }

    if (curr_byte == ANSI_ESCAPE) {
      curr_byte = (uint8_t)getch();

      if (curr_byte != '[') {
        continue;
      }

      switch (getch()) {
        // arrow up
        case 'A':
          if (node == NULL) {
            if (last_line_node != NULL) {
              node = last_line_node;
              cursor_pos = node->mut_line.length;
              displayed_cursor_pos =
                  strlen_utf8(node->mut_line.data, node->mut_line.length) +
                  prompt_length;

              width = get_terminal_width();
              size_t moves_up = characters_printed / width;
              if (moves_up) {
                printf("\033[%zuA", moves_up);
              }
              characters_printed = displayed_cursor_pos;
              DRAW_LINE(node->mut_line);
              (void)fflush(stdout);
            }
          } else if (node->p_prev != NULL) {
            node = node->p_prev;
            cursor_pos = node->mut_line.length;
            displayed_cursor_pos =
                strlen_utf8(node->mut_line.data, node->mut_line.length) +
                prompt_length;

            width = get_terminal_width();
            size_t moves_up = characters_printed / width;
            if (moves_up) {
              printf("\033[%zuA", moves_up);
            }
            characters_printed = displayed_cursor_pos;
            DRAW_LINE(node->mut_line);
            (void)fflush(stdout);
          }
          continue;
        // arrow down
        case 'B':
          if (node != NULL && node->p_next != NULL) {
            node = node->p_next;
            cursor_pos = node->mut_line.length;
            displayed_cursor_pos =
                strlen_utf8(node->mut_line.data, node->mut_line.length) +
                prompt_length;

            width = get_terminal_width();
            size_t moves_up = characters_printed / width;
            if (moves_up) {
              printf("\033[%zuA", moves_up);
            }
            characters_printed = displayed_cursor_pos;
            DRAW_LINE(node->mut_line);
            (void)fflush(stdout);
          } else {
            node = NULL;
            cursor_pos = line.length;
            displayed_cursor_pos =
                strlen_utf8(line.data, line.length) + prompt_length;

            width = get_terminal_width();
            size_t moves_up = characters_printed / width;
            if (moves_up) {
              printf("\033[%zuA", moves_up);
            }
            characters_printed = displayed_cursor_pos;
            DRAW_LINE(line);
            (void)fflush(stdout);
          }
          continue;
        // right arrow
        case 'C':
          if (cursor_pos < current_line->length) {
            cursor_pos += traverse_forward_utf8(
                current_line->data, current_line->length, cursor_pos
            );

            CURSOR_RIGHT;
            (void)fflush(stdout);
          }
          continue;
        // left arrow
        case 'D':
          if (cursor_pos > 0) {
            cursor_pos -= traverse_back_utf8(current_line->data, cursor_pos);

            CURSOR_LEFT;
            (void)fflush(stdout);
          }
          continue;
        case '1':
          if (getch() == ';') {
            if (getch() == '5') {
              const uint8_t arrow_char = (uint8_t)getch();
              // ctrl right arrow
              if (arrow_char == 'C') {
                if (cursor_pos < current_line->length) {
                  size_t move_right = 1;

                  cursor_pos += traverse_forward_utf8(
                      current_line->data, current_line->length, cursor_pos
                  );

                  while (cursor_pos <= current_line->length - 1 &&
                         current_line->data[cursor_pos] != ' ') {
                    cursor_pos += traverse_forward_utf8(
                        current_line->data, current_line->length, cursor_pos
                    );
                    move_right++;
                  }

                  displayed_cursor_pos += move_right;
                  CURSOR_RIGHT_N(move_right);
                  (void)fflush(stdout);
                }

                continue;
              }
              // ctrl left arrow
              if (arrow_char == 'D') {
                if (cursor_pos > 0) {
                  size_t move_left = 1;

                  cursor_pos -=
                      traverse_back_utf8(current_line->data, cursor_pos);

                  while (cursor_pos > 0 &&
                         current_line->data[cursor_pos - 1] != ' ') {
                    cursor_pos -=
                        traverse_back_utf8(current_line->data, cursor_pos);
                    move_left++;
                  }

                  displayed_cursor_pos -= move_left;
                  CURSOR_LEFT_N(move_left);
                  (void)fflush(stdout);
                }

                continue;
              }
            }
          }
          break;
        case '3':
          // delete key
          if (getch() == '~') {
            if (cursor_pos < current_line->length) {
              line_delete(current_line, cursor_pos);
              characters_printed--;
            }
            // should probably refactor to not use goto, but, i mean, it
            // works...
            goto draw_line;
          }
          break;
        // home key
        case 'H': {
          cursor_pos = 0;
          size_t moves_up = displayed_cursor_pos / width;
          displayed_cursor_pos = prompt_length;
          if (moves_up > 0) {
            printf("\033[%zuA", moves_up);
          }
          printf("\r\033[%uC", prompt_length);
          (void)fflush(stdout);
          continue;
        }
        // end key
        case 'F': {
          cursor_pos = current_line->length;
          // eol is end of line
          size_t line_len =
              strlen_utf8(current_line->data, current_line->length) +
              prompt_length;
          // this is real code written by sane individuals
          size_t moves_down = (line_len - displayed_cursor_pos) / width;
          displayed_cursor_pos = line_len;
          if (moves_down > 0) {
            printf("\033[%zuB", moves_down);
          }
          printf("\r\033[%zuC", line_len % width);
          (void)fflush(stdout);
          continue;
        }
        // shift+tab
        case 'Z': {
          show_matches = !show_matches;

          goto draw_line;
        }

        default:
          continue;
      }
    }

    // backspace
    if (curr_byte == ASCII_DEL) {
      if (cursor_pos > 0) {
        const size_t bytes_removed = line_backspace(current_line, cursor_pos);
        cursor_pos -= bytes_removed;
        characters_printed--;
        CURSOR_LEFT;
        goto draw_line;
      }

      continue;
    }

    if (curr_byte == '\t') {
      size_t word_start = cursor_pos - 1;

      for (; word_start > 0; word_start--) {
        if (current_line->data[word_start] == ' ') {
          word_start++;
          break;
        }
      }

      char *word = (char *)current_line->data + word_start;
      size_t word_len = cursor_pos - word_start;
      size_t bytes_written = 0;

      if (word_len == 0) {
        continue;
      }

      strings_t matches = {0};

      if (word_start == 0 && memchr(word, '/', word_len) == NULL) {
        get_command_matches(&matches, word, word_len);
      } else {
        get_file_matches(&matches, word, word_len);
      }

      if (matches.length == 0) {
        continue;
      }

      if (matches.length == 1) {
        size_t match_len = strlen(matches.data[0]);

        if (match_len > word_len) {
          bytes_written = match_len - word_len;

          line_insert_bulk(
              current_line,
              cursor_pos,
              (uint8_t *)matches.data[0] + word_len,
              bytes_written
          );
        }

        free(matches.data[0]);
        VECTOR_DESTROY(matches);
      } else {
        size_t i;

        for (i = 0;; i++) {
          for (size_t j = 0; j < matches.length - 1; j++) {
            if (matches.data[j][i] != matches.data[j + 1][i]) {
              goto leave;
            }
            if (matches.data[j][i] == '\0' || matches.data[j + 1][i] == '\0') {
              goto leave;
            }
          }
        }

      leave: {
        if (i > word_len) {
          bytes_written = i - word_len;
          line_insert_bulk(
              current_line,
              cursor_pos,
              (uint8_t *)matches.data[0] + word_len,
              bytes_written
          );
        } else {
          sort_strings(&matches);
          putchar('\n');
          pretty_print_strings(matches.data, matches.length);
          printf(
              "\n\033[s%s%.*s\033[u",
              prompt,
              (int)current_line->length,
              (char *)current_line->data
          );
          size_t moves_down =
              ((displayed_cursor_pos + displayed_cursor_pos) / width) -
              (displayed_cursor_pos / width);
          if (moves_down > 0) {
            printf("\033[%zuB", moves_down);
          }
          printf("\r\033[%zuC", displayed_cursor_pos % width);
          (void)fflush(stdout);
        }

        for (size_t j = 0; j < matches.length; j++) {
          free(matches.data[j]);
        }
        VECTOR_DESTROY(matches);
      }
      }

      if (bytes_written) {
        const size_t n =
            strlen_utf8(current_line->data + cursor_pos, bytes_written);
        displayed_cursor_pos += n;
        CURSOR_RIGHT_N(n);
        cursor_pos += bytes_written;

        goto draw_line;
      }
      continue;
    }

    // clear screen
    if (curr_byte == '\f') {
      printf("\033[H\033[2J");
      CURSOR_RIGHT_N(displayed_cursor_pos);
      goto draw_line;
    }

    // only add character to buffer if it is displayable ascii (or utf-8)
    if (curr_byte >= ' ') {
      line_insert(current_line, curr_byte, cursor_pos);
      cursor_pos++;

      if (!is_continuation_byte_utf8(curr_byte)) {
        CURSOR_RIGHT;
        characters_printed++;
      }
    }

  draw_line:

    (void)fputs(ANSI_CURSOR_POS_SAVE, stdout);

    width = get_terminal_width();

    size_t moves_up = displayed_cursor_pos / width;
    if (moves_up > 0) {
      printf("\033[%zuA", moves_up);
    }

    DRAW_LINE(*current_line);

    if (show_matches) {
      strings_t matches = {0};
      get_matches(&matches, current_line, cursor_pos);
      if (matches.length) {
        sort_strings(&matches);
        printf("\033[2m\n");
        pretty_print_strings(matches.data, matches.length);
        printf("\033[0m");
      }
      VECTOR_DESTROY(matches);
    }

    (void)fputs(ANSI_CURSOR_POS_RESTORE, stdout);

    (void)fflush(stdout);
  }

  line_node_t *new_node = malloc(sizeof(line_node_t));
  if (node != NULL) {
    VECTOR_PUSH(node->mut_line, '\0');
    line_copy(&line, &node->mut_line);
    node->mut_line.length--;
    line.length--;
  } else {
    VECTOR_PUSH(line, '\0');
    line.length--;
  }

  new_node->mut_line = line;
  new_node->const_line = malloc(line.length + 1);
  strcpy((char *)new_node->const_line, (char *)line.data);
  new_node->const_line_len = line.length;

  new_node->p_next = NULL;
  new_node->p_prev = last_line_node;
  if (last_line_node != NULL) {
    last_line_node->p_next = new_node;
  } else {
    root_line_node = new_node;
  }
  last_line_node = new_node;

  printf("\n");

  if (prompt_var != NULL) {
    free(prompt);
  }

  return line.data;
}
