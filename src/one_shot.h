#ifndef ONE_SHOT_H
#define ONE_SHOT_H

#include <stdint.h>

void one_shot_init(const uint8_t *line);

const uint8_t *one_shot_reader(void);

#endif
