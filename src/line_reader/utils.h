#ifndef LINE_UTILS_H
#define LINE_UTILS_H

#include <stddef.h>

#include "line_reader.h"

// just a quick short hand for printing out a line.
#define PRINT_LINE(line)                                                       \
  do {                                                                         \
    fwrite((line).data, sizeof(*(line).data), (line).length, stdout);          \
    fputs(" ", stdout);                                                        \
  } while (0)

// returned by getch when a sigint interrupted the read.
#define SIGINT_ON_READ -1

typedef VECTOR(char *) matches_t;

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

/**
 * @brief prints the prompt followed by the line, the cursor position is
 * preserved.
 * @param prompt the prompt to print, currently this is printed as is, and thus
 * is not expanded.
 * @param line the line to print
 */
void draw_line(const char *const prompt, const line_t *const line);

/**
 * @brief searches the files in path and adds the files that begin with prefix
 * to the matches vector.
 * @param matches the vector to add the matches to.
 * @param path the path to search for files.
 * @param prefix if a file in path begins with this prefix, it will be added to
 * the matches vector.
 * @param prefix_len the length of the prefix
 */
void add_path_matches(
    matches_t *matches,
    const char *const path,
    const char *const prefix,
    const size_t prefix_len
);

unsigned short get_terminal_width(void);

#endif
