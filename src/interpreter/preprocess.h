#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <stdbool.h>
#include <stdint.h>

#include "../vec_types.h"

extern char *pp_error_msg;

buf_t *preprocess(const uint8_t *source);

#endif
