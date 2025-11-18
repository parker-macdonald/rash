#ifndef PROMPT_H
#define PROMPT_H

#include <stddef.h>

#include "../vector.h"

struct prompt {
  char *data;
  size_t length;
  unsigned int characters;
};

typedef VECTOR(struct prompt) prompts_t;

prompts_t get_prompts(const char *const prompt);

#endif
