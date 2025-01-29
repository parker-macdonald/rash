#include "vector.h"
#include <assert.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

typedef VECTOR(char) line_t;

char getch(void) {
  struct termios oldt, newt;
  char ch;

  tcgetattr(STDIN_FILENO, &oldt);          // Get the current terminal settings
  newt = oldt;                             // Copy them to a new variable
  newt.c_lflag &= ~(ICANON | ECHO);        // Disable canonical mode and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &newt); // Set the new settings

  ch = getchar(); // Read a single character

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original settings
  return ch;
}

void line_insert(line_t *const line, const char c,
                 const unsigned int cursor_pos) {
  if (cursor_pos == line->length) {
    VECTOR_PUSH((*line), c);
    return;
  }

  char old = line->data[cursor_pos];
  line->data[cursor_pos] = c;

  for (size_t i = cursor_pos + 1; i < line->length; i++) {
    char old2 = line->data[i];

    line->data[i] = old;

    old = old2;
  }

  VECTOR_PUSH((*line), old);
}

void line_remove(line_t *const line, const unsigned int cursor_pos) {
  line->length--;

  for (size_t i = cursor_pos - 1; i < line->length; i++) {
    line->data[i] = line->data[i + 1];
  }
}

char *readline(char *data, const char *const prompt) {
  printf("%s", prompt);

  line_t line;

  line.capacity = 16;
  line.length = 0;
  if (data == NULL) {
    line.data = malloc(sizeof(*line.data) * 16);
  } else {
    line.data = data;
  }

  unsigned int cursor_pos = 0;

  char c;
  for (;;) {
    c = getch();

    if (c == '\n' || c == EOF) {
      break;
    }

    if (c == '\033') {
      c = getch();
      assert(c == '[');

      switch (getch()) {
        // right arrow
      case 'C':
        if (cursor_pos < line.length) {
          cursor_pos++;
          printf("\033[C");
        }
        break;
      // left arrow
      case 'D':
        if (cursor_pos > 0) {
          cursor_pos--;
          printf("\033[D");
        }
        break;
      }

      continue;
      // backspace
    } else if (c == 0x7f) {
      if (cursor_pos > 0) {
        line_remove(&line, cursor_pos);
        cursor_pos--;
        printf("\033[D");
      }
    } else {
      line_insert(&line, c, cursor_pos);
      cursor_pos++;
      printf("\033[C");
    }

    // save cursor pos
    printf("\033[s");

    // remove line
    printf("\033[2K\r");
    printf("%s", prompt);
    for (size_t i = 0; i < line.length; i++) {
      printf("%c", line.data[i]);
    }
    printf("\033[u");
  }

  VECTOR_PUSH(line, '\0');

  printf("\n");

  return line.data;
}
