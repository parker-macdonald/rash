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

#include "jobs.h"
#include "lib/ansi.h"
#include "lib/vector.h"

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

  for (size_t i = 0; i < length; i++) {
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
  uint8_t byte = 0;
  ssize_t nread;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(unsigned int)(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  // this is to allow the read system call to know a sigint occured.
  dont_restart_on_sigint();

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
      restart_on_sigint();
      tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings

      return SIGINT_ON_READ;
    }
  }

  restart_on_sigint();
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
