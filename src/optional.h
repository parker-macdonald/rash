#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <stdbool.h>

#define OPTIONAL(type)                                                         \
  struct {                                                                     \
    type value;                                                                \
    bool has_value;                                                            \
  }

#endif
