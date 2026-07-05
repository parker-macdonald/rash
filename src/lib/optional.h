#ifndef OPTIONAL_H
#define OPTIONAL_H

#define OPTIONAL(type)                                                         \
  struct {                                                                     \
    type value;                                                                \
    _Bool has_value;                                                           \
  }

#endif
