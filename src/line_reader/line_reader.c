#include "line_reader.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "lib/vector.h"
#include "line_reader/actions.h"
#include "line_reader/history.h"
#include "line_reader/prompt.h"
#include "line_reader_struct.h"
#include "shell_vars.h"

static LineReader reader;

void line_reader_init(void) {
  reader.buffer._capacity = 0;
  reader.buffer.length = 0;
  reader.buffer.data = 0;

  reader.buffer_offset = 0;

  reader.active_buffer = &reader.buffer;

  reader.history_root = NULL;
  reader.history_end = NULL;
  reader.history_curr = NULL;

  reader.cursor_pos = reader.prompt_length;

  actions_default(&reader.acts);
}

void line_reader_destroy(void) {
  VECTOR_DESTROY(reader.buffer);
  history_clear(&reader);
}

const Buffer *line_reader_read_void(void *_) {
  (void)_;

  return line_reader_read();
}

static struct termios oldt = {0};
static void reader_begin(void) {
  const char *prompt = var_get("PS1");

  if (prompt == NULL) {
    reader.prompt_length = 2;
    reader.prompt = strdup("$ ");
  } else {
    reader.prompt_length = get_prompt(&reader.prompt, prompt);
  }

  VECTOR_INIT(reader.buffer);
  reader.active_buffer = &reader.buffer;
  reader.buffer_offset = 0;
  reader.cursor_pos = reader.prompt_length;

  printf("%s", reader.prompt);
  (void)fflush(stdout);

  struct termios newt = {0};

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(unsigned int)(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

static void reader_end(void) {
  free(reader.prompt);
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

const Buffer *line_reader_read(void) {
  reader_begin();

  while (1) {
    int status = preform_action(&reader);

    if (status < 0) {
      reader_end();
      return NULL;
    }

    if (status > 0) {
      break;
    }
  }

  reader_end();
  return &reader.buffer;
}

void line_reader_hist_print(int count) {
  history_print(&reader, count);
}

void line_reader_hist_clear(void) {
  history_clear(&reader);
}
