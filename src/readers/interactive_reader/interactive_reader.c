#include "interactive_reader.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib/buffer.h"
#include "lib/utf_8.h"
#include "rash.h"
#include "readers/interactive_reader/actions.h"
#include "readers/interactive_reader/history.h"
#include "readers/interactive_reader/raw_mode.h"
#include "readers/interactive_reader/types.h"
#include "shell_vars/shell_vars.h"

void interactive_reader_init(InteractiveReader *self) {
  self->history._capacity = 0;
  self->history.length = 0;
  self->history.data = NULL;

  actions_default(&self->action_set);
}

void interactive_reader_destroy(InteractiveReader *self) {
  buffer_destroy(&self->buffer);
  history_clear(self);
}

static void reader_begin(InteractiveReader *self) {
  enable_raw_mode();

  char *prompt_cstr = var_eval_to_string(buffer_cstr(&rash_instance_get()->interactive_prompt));

  if (prompt_cstr == NULL) {
    self->prompt = buffer_from_cstr("$ ");
  } else {
    self->prompt = buffer_from_cstr(prompt_cstr);
  }

  free(prompt_cstr);
  self->prompt_length = utf8_count_codepoint(&self->prompt);

  self->buffer = buffer_create(16);
  self->active_buffer = &self->buffer;
  self->buffer_offset = 0;
  self->cursor_pos = self->prompt_length;

  self->history_curr = self->history.length;

  printf("\r%.*s", (int)self->prompt.length, self->prompt.char_ptr);
  (void)fflush(stdout);
}

static void reader_end(InteractiveReader *self) {
  buffer_destroy(&self->prompt);
  disable_raw_mode();
}

const Buffer *interactive_reader_read(InteractiveReader *self) {
  reader_begin(self);

  while (1) {
    int status = preform_action(self);

    if (status < 0) {
      reader_end(self);
      return NULL;
    }

    if (status > 0) {
      break;
    }
  }

  reader_end(self);
  return &self->buffer;
}

void interactive_reader_hist_print(const InteractiveReader *self, int count) {
  history_print(self, count);
}

void interactive_reader_hist_clear(InteractiveReader *self) {
  history_clear(self);
}
