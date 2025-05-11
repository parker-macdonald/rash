#include "line_reader.h"

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "../ansi.h"
#include "../jobs.h"
#include "../utf_8.h"
#include "../vector.h"
#include "modify_line.h"

static_assert(sizeof(char) == sizeof(uint8_t),
              "char is not one byte in size, god save you...");

typedef struct line_node {
  struct line_node *p_next;
  struct line_node *p_prev;
  line_t line;
} line_node_t;

#define PRINT_LINE(line)                                                       \
  fwrite((line).data, sizeof(*(line).data), (line).length, stdout)

#define RECV_SIGINT -1

static line_node_t *root_line_node = NULL;
static line_node_t *last_line_node = NULL;

void line_reader_destroy(void) {
  line_node_t *node = last_line_node;

  while (node != NULL) {
    VECTOR_DESTROY(node->line);

    line_node_t *prev_node = node->p_prev;

    free(node);
    node = NULL;

    node = prev_node;
  }

  root_line_node = NULL;
  last_line_node = NULL;
}

// returns a uint8_t casted to an int or RECV_SIGINT, when a sigint is recieved.
static int getch(void) {
  struct termios oldt;
  struct termios newt;
  uint8_t byte;
  ssize_t nread;

  tcgetattr(STDIN_FILENO, &oldt); // Get the current terminal settings
  newt = oldt;                    // Copy them to a new variable
  newt.c_lflag &=
      ~(unsigned int)(ICANON | ECHO);      // Disable canonical mode and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &newt); // Set the new settings

  for (;;) {
    errno = 0;
    nread = read(STDIN_FILENO, &byte, sizeof(byte));

    // adding a null terminator to the line buffer will fuck up the lexer
    if (nread == 1 && byte == '\0') {
      continue;
    }
    if (errno != EINTR) {
      break;
    }
    if (recv_sigint == 1) {
      recv_sigint = 0;
      tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings

      return RECV_SIGINT;
    }
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings

  if (nread == 0) {
    return ASCII_END_OF_TRANSMISSION;
  }

  if (nread == -1) {
    perror("read");
    return ASCII_END_OF_TRANSMISSION;
  }

  return (int)byte;
}

static void draw_line(const char *const prompt, const line_t *const line) {
  // the old line reader used to just redraw what changed, but that had lots of
  // bugs so now i'm just redrawing the whole line
  fputs(ANSI_REMOVE_FULL_LINE, stdout);
  // reset cursor to start of line
  fputs("\r", stdout);

  fputs(prompt, stdout);

  PRINT_LINE(*line);

  fflush(stdout);
}

void clear_history(void) {
  line_node_t *node = root_line_node;

  while (node != NULL) {
    line_node_t *next_node = node->p_next;

    free(node);

    node = next_node;
  }
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
      printf("%5u  ", i);
      PRINT_LINE(node->line);
      fputs("\n", stdout);
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

  for (int i = count - 1; node != NULL; i--) {
    printf("%5d  ", length - i);
    PRINT_LINE(node->line);
    fputs("\n", stdout);
    node = node->p_next;
  }
}

static char *splitpath(uint8_t *path, const size_t length, char **pathname) {
  if (length == 0) {
    char *dirname = malloc(2);
    strcpy(dirname, ".");
    *pathname = (char *)path;
    return dirname;
  }

  size_t last_slash = 0;

  for (size_t i = 0; i < length; i++) {
    if (path[i] == '/') {
      last_slash = i;
    }
  }

  if (last_slash == 0) {
    char *dirname = malloc(2);
    strcpy(dirname, ".");
    *pathname = (char *)path;
    return dirname;
  }

  VECTOR(char) dirname;
  VECTOR_INIT(dirname);

  bool just_slashed = false;

  for (size_t i = 0; i < last_slash; i++) {
    if (path[i] == '/') {
      if (!just_slashed) {
        VECTOR_PUSH(dirname, '/');
      }

      just_slashed = true;
      continue;
    }

    just_slashed = false;
    VECTOR_PUSH(dirname, (char)path[i]);
  }

  VECTOR_PUSH(dirname, '\0');

  *pathname = (char *)path + last_slash + 1;
  return dirname.data;
}

const uint8_t *readline(void) {
  char *prompt = getenv("PS1");
  if (prompt == NULL) {
    prompt = "$ ";
  }

  printf("%s", prompt);
  fflush(stdout);

  line_node_t *node = NULL;

  line_t line;
  VECTOR_INIT(line);

  size_t cursor_pos = 0;

  uint8_t curr_byte;
  for (;;) {
    int ch = getch();

    // this is sent by the sigint handler to let the line reader know the user
    // pressed ctrl-c to trigger a SIGINT.
    if (ch == RECV_SIGINT) {
      printf("\n%s", prompt);
      fflush(stdout);
      line.length = 0;
      cursor_pos = 0;
      continue;
    }

    curr_byte = (uint8_t)ch;
    fputs(ANSI_REMOVE_BELOW_CURSOR, stdout);

    if (curr_byte == ASCII_END_OF_TRANSMISSION) {
      printf("\n");
      VECTOR_DESTROY(line);
      line_reader_destroy();
      return NULL;
    }

    // readonly line to get length or data info from. when the user is going
    // through history, node contains the line they are viewing whereas line
    // contains the buffer the user is currently editting
    line_t *line_to_read = node == NULL ? &line : &node->line;

    if (curr_byte == '\n') {
      if (line_to_read->length != 0) {
        break;
      }

      printf("\n%s", prompt);
      fflush(stdout);
      continue;
    }

    if (curr_byte == '\t') {
      size_t path_start;

      for (path_start = line_to_read->length - 1; path_start > 0;
           path_start--) {
        if (isspace((int)line_to_read->data[path_start])) {
          path_start++;
          break;
        }
      }

      if (path_start == 0) {
        const char *const path = getenv("PATH");

        if (path == NULL) {
          continue;
        }

        char *path2 = strdup(path);

        char *path_part = strtok(path2, ":");
        VECTOR(char *) matches;
        VECTOR_INIT(matches);

        do {
          DIR *dir = opendir(path_part);
          if (dir == NULL) {
            continue;
          }
          struct dirent *ent;

          while ((ent = readdir(dir)) != NULL) {
            if (strncmp((char *)line_to_read->data, ent->d_name,
                        line_to_read->length) == 0) {
              bool already_contained = false;
              for (size_t i = 0; i < matches.length; i++) {
                if (strcmp(matches.data[i], ent->d_name) == 0) {
                  already_contained = true;
                  break;
                }
              }

              if (!already_contained) {
                VECTOR_PUSH(matches, strdup(ent->d_name));
              }
            }
          }

          closedir(dir);
        } while ((path_part = strtok(NULL, ":")) != NULL);

        free(path2);

        if (matches.length == 1) {
          if (node != NULL) {
            line_copy(&line, &node->line);
            node = NULL;
          }
          for (size_t i = line_to_read->length; matches.data[0][i] != '\0';
               i++) {
            VECTOR_PUSH(line, (uint8_t)matches.data[0][i]);
            cursor_pos++;

            if (!is_continuation_byte_utf8((uint8_t)matches.data[0][i])) {
              fputs(ANSI_CURSOR_RIGHT, stdout);
            }
          }

          for (size_t i = 0; i < matches.length; i++) {
            free(matches.data[i]);
          }

          VECTOR_DESTROY(matches);

          VECTOR_PUSH(line, ' ');
          cursor_pos++;

          fputs(ANSI_CURSOR_RIGHT, stdout);
          goto draw_line;
        }

        if (matches.length == 0) {
          VECTOR_DESTROY(matches);
          continue;
        }

        unsigned int max_len = 0;
        for (size_t i = 0; i < matches.length; i++) {
          const unsigned int new_len = (unsigned int)strlen(matches.data[i]);

          if (new_len > max_len) {
            max_len = new_len;
          }
        }

        // add some padding
        max_len += 2;

        unsigned int width;

        struct winsize win;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) != -1) {
          width = win.ws_col;
        } else {
          // assume 80 columns if we cant get the terminal size
          width = 80;
        }
        const unsigned int col = width / max_len;

        printf("%s\n", ANSI_CURSOR_POS_SAVE);

        for (unsigned int i = 0; i < matches.length; i++) {
          printf("%-*s", max_len, matches.data[i]);
          free(matches.data[i]);

          if ((i + 1) % col == 0) {
            printf("\n");
          }
        }

        printf("\n%s", ANSI_CURSOR_POS_RESTORE);

        VECTOR_DESTROY(matches);

        goto draw_line;
      }

      char *pathname;
      char *dirname = splitpath(line_to_read->data + path_start,
                                line_to_read->length - path_start, &pathname);

      DIR *dir = opendir(dirname);
      free(dirname);

      if (dir == NULL) {
        continue;
      }

      struct dirent *ent;

      size_t pathname_len =
          line_to_read->length + (size_t)line_to_read->data - (size_t)pathname;
      while ((ent = readdir(dir)) != NULL) {
        if (strncmp(pathname, ent->d_name, pathname_len) == 0) {
          if (node != NULL) {
            line_copy(&line, &node->line);
            node = NULL;
          }
          for (size_t i = pathname_len; ent->d_name[i] != '\0'; i++) {
            VECTOR_PUSH(line, (uint8_t)ent->d_name[i]);
            cursor_pos++;

            if (!is_continuation_byte_utf8((uint8_t)ent->d_name[i])) {
              fputs(ANSI_CURSOR_RIGHT, stdout);
            }
          }

          VECTOR_PUSH(line, ent->d_type == DT_DIR ? '/' : ' ');
          cursor_pos++;
          fputs(ANSI_CURSOR_RIGHT, stdout);

          break;
        }
      }

      closedir(dir);
      goto draw_line;
    }

    if (curr_byte == ANSI_START_CHAR) {
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
              cursor_pos = node->line.length;
              draw_line(prompt, &node->line);
            }
          } else if (node->p_prev != NULL) {
            node = node->p_prev;
            cursor_pos = node->line.length;
            draw_line(prompt, &node->line);
          }
          continue;
        // arrow down
        case 'B':
          if (node != NULL && node->p_next != NULL) {
            node = node->p_next;
            cursor_pos = node->line.length;
            draw_line(prompt, &node->line);
          } else {
            node = NULL;
            cursor_pos = line.length;
            draw_line(prompt, &line);
          }
          continue;
        // right arrow
        case 'C':
          if (cursor_pos < line_to_read->length) {
            cursor_pos += traverse_forward_utf8(
                line_to_read->data, line_to_read->length, cursor_pos);

            fputs(ANSI_CURSOR_RIGHT, stdout);
            fflush(stdout);
          }
          continue;
        // left arrow
        case 'D':
          if (cursor_pos > 0) {
            cursor_pos -= traverse_back_utf8(line_to_read->data, cursor_pos);

            fputs(ANSI_CURSOR_LEFT, stdout);
            fflush(stdout);
          }
          continue;
        case '1':
          if (getch() == ';') {
            if (getch() == '5') {
              const uint8_t arrow_char = (uint8_t)getch();
              // shift right arrow
              if (arrow_char == 'C') {
                if (cursor_pos < line_to_read->length) {
                  cursor_pos += traverse_forward_utf8(
                      line_to_read->data, line_to_read->length, cursor_pos);
                  fputs(ANSI_CURSOR_RIGHT, stdout);

                  while (cursor_pos <= line_to_read->length - 1 &&
                         line_to_read->data[cursor_pos] != ' ') {
                    cursor_pos += traverse_forward_utf8(
                        line_to_read->data, line_to_read->length, cursor_pos);
                    fputs(ANSI_CURSOR_RIGHT, stdout);
                  }

                  fflush(stdout);
                }

                continue;
              }
              // shift left arrow
              if (arrow_char == 'D') {
                if (cursor_pos > 0) {
                  cursor_pos -=
                      traverse_back_utf8(line_to_read->data, cursor_pos);
                  fputs(ANSI_CURSOR_LEFT, stdout);

                  while (cursor_pos > 0 &&
                         line_to_read->data[cursor_pos] != ' ') {
                    cursor_pos -=
                        traverse_back_utf8(line_to_read->data, cursor_pos);
                    fputs(ANSI_CURSOR_LEFT, stdout);
                  }

                  fflush(stdout);
                }

                continue;
              }
            }
          }
          break;
        case '3':
          // delete key
          if (getch() == '~') {
            if (cursor_pos < line_to_read->length) {
              if (node != NULL) {
                line_copy(&line, &node->line);
                node = NULL;
              }
              line_delete(&line, cursor_pos);
            }
            // should probably refactor to not use goto, but, i mean, it
            // works...
            goto draw_line;
          }
          break;
        default:
          continue;
      }
    }

    // backspace
    if (curr_byte == ASCII_DEL) {
      if (cursor_pos > 0) {
        if (node != NULL) {
          line_copy(&line, &node->line);
          node = NULL;
        }
        const size_t bytes_removed = line_backspace(&line, cursor_pos);
        cursor_pos -= bytes_removed;
        fputs(ANSI_CURSOR_LEFT, stdout);
        goto draw_line;
      }

      continue;
    }

    if (node != NULL) {
      line_copy(&line, &node->line);
      node = NULL;
    }

    line_insert(&line, curr_byte, cursor_pos);
    cursor_pos++;

    if (!is_continuation_byte_utf8(curr_byte)) {
      fputs(ANSI_CURSOR_RIGHT, stdout);
    }

  draw_line:

    fputs(ANSI_CURSOR_POS_SAVE, stdout);

    fputs(ANSI_REMOVE_FULL_LINE, stdout);
    // reset cursor to start of line
    printf("\r");

    fputs(prompt, stdout);
    PRINT_LINE(line);

    fputs(ANSI_CURSOR_POS_RESTORE, stdout);

    fflush(stdout);
  }

  VECTOR_PUSH(line, '\0');

  line_node_t *new_node = malloc(sizeof(line_node_t));
  if (node != NULL) {
    line_copy(&line, &node->line);
  }

  new_node->line = line;

  new_node->p_next = NULL;
  new_node->p_prev = last_line_node;
  if (last_line_node != NULL) {
    last_line_node->p_next = new_node;
  } else {
    root_line_node = new_node;
  }
  last_line_node = new_node;

  printf("\n");

  return line.data;
}
