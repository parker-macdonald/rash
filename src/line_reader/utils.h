#ifndef LINE_UTILS_H
#define LINE_UTILS_H

#include <stddef.h>

#include "line_reader.h"

// just a quick short hand for printing out a line.
#define PRINT_LINE(line)                                                       \
  do {                                                                         \
    fwrite((line).data, sizeof(*(line).data), (line).length, stdout);          \
    fputs(" \033[D", stdout);                                                  \
  } while (0)

// returned by getch when a sigint interrupted the read.
#define SIGINT_ON_READ -1

/**
 * @brief prints all the strings in the strings array in a nice format that
 * scales to the terminal size.
 * @param strings the list of strings to print.
 * @param length the length of the list of strings
 */
void pretty_print_strings(char *const strings[], const size_t length);

/**
 * @brief reads a byte from stdin without printing it to the screen or moving
 * the cursor.
 * @return returns a uint8_t casted to an int or RECV_SIGINT, when a sigint is
 * recieved.
 */
int getch(void);

unsigned short get_terminal_width(void);

#endif
