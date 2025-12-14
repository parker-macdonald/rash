#include "one_shot.h"

#include <stddef.h>
#include <stdint.h>

static const uint8_t *data = NULL;

void one_shot_init(const uint8_t *line) {
  data = line;
}

const uint8_t *one_shot_reader(void) {
  const uint8_t *temp = data;
  data = NULL;
  return temp;
}
