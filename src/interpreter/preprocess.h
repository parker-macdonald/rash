#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <stdbool.h>
#include <stdint.h>

#include "../vec_types.h"

buf_t *preprocess(const uint8_t *source, bool print_errors);

#endif
