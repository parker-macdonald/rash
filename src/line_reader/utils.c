#include "utils.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "../ansi.h"
#include "../jobs.h"
#include "../vector.h"
#include "line_reader.h"

unsigned short get_terminal_width(void) {
  struct winsize win;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) != -1) {
    return win.ws_col;
  } else {
    // assume 80 columns if we cant get the terminal size
    return 80;
  }
}

void pretty_print_strings(char *const strings[], const size_t length) {
  unsigned int max_len = 0;
  for (size_t i = 0; i < length; i++) {
    const unsigned int new_len = (unsigned int)strlen(strings[i]);

    if (new_len > max_len) {
      max_len = new_len;
    }
  }

  // add some padding
  max_len += 2;

  const unsigned int col = (unsigned int)get_terminal_width() / max_len;

  printf("\n");

  for (unsigned int i = 0; i < length; i++) {
    printf("%-*s", max_len, strings[i]);

    if ((i + 1) % col == 0) {
      printf("\n");
    }
  }

  printf("\n");
}

int getch(void) {
  struct termios oldt = {0};
  struct termios newt = {0};
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

      return SIGINT_ON_READ;
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

void draw_line(const char *const prompt, const line_t *const line) {
  // the old line reader used to just redraw what changed, but that had lots of
  // bugs so now i'm just redrawing the whole line
  fputs(ANSI_REMOVE_FULL_LINE, stdout);
  // reset cursor to start of line
  fputs("\r", stdout);

  fputs(prompt, stdout);

  PRINT_LINE(*line);

  fflush(stdout);
}

void add_path_matches(
    matches_t *matches,
    const char *const path,
    const char *const prefix,
    const size_t prefix_len
) {
  DIR *dir = opendir(path);
  if (dir == NULL) {
    return;
  }
  struct dirent *ent;

  while ((ent = readdir(dir)) != NULL) {
    if (strncmp(prefix, ent->d_name, prefix_len) == 0) {
      bool already_contained = false;

      for (size_t i = 0; i < matches->length; i++) {
        if (strcmp(matches->data[i], ent->d_name) == 0) {
          already_contained = true;
          break;
        }
      }

      if (!already_contained) {
        VECTOR_PUSH(*matches, strdup(ent->d_name));
      }
    }
  }

  closedir(dir);
}
