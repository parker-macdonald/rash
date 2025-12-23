#include "one_shot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool already_run = false;

const uint8_t *one_shot_reader(void *data) {
  if (already_run) {
    already_run = false;
    return NULL;
  }

  already_run = true;
  return (const uint8_t *)data;
}
