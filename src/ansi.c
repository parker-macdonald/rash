#include <stdio.h>

const char ANSI_START_CHAR = '\033';
const char ASCII_END_OF_TRANSMISSION = '\04';

const char ANSI_CURSOR_LEFT[] = "\033[D";
const char ANSI_CURSOR_RIGHT[] = "\033[C";

const char ANSI_REMOVE_FULL_LINE[] = "\033[2K";

const char ANSI_CURSOR_POS_SAVE[] = "\033[s";
const char ANSI_CURSOR_POS_RESTORE[] = "\033[u";

void ansi_cursor_right(const unsigned int num) {
  if (num == 0) {
    return;
  }

  if (num == 1) {
    fputs("\033[C", stdin);
    return;
  }

  printf("\033[%uC", num);
}

void ansi_cursor_left(const unsigned int num) {
  if (num == 0) {
    return;
  }

  if (num == 1) {
    fputs("\033[D", stdin);
    return;
  }

  printf("\033[%uD", num);
}
