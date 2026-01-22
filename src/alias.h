#ifndef ALIAS_H
#define ALIAS_H

#include <stddef.h>

#include "optional.h"

typedef OPTIONAL(struct alias_t {
  const size_t length;
  char **value;
}) alias_or_none;

void alias_set(const char *key, char *value[], size_t length);

alias_or_none alias_get(const char *key);

int alias_unset(const char *key);

void alias_print(void);

#endif
