#include "shlvl.h"

#include <stdlib.h>

#include "lib/dynamic_sprintf.h"
#include "lib/parse.h"

void set_shlvl(void) {
  char *shlvl = getenv("SHLVL");

  if (shlvl == NULL) {
    setenv("SHLVL", "1", 1);
    return;
  }

  OptionUnsignedLong num = parse_unsigned_long(shlvl);

  if (!num.has_value) {
    setenv("SHLVL", "1", 1);
    return;
  }

  shlvl = dynamic_sprintf("%lu", num.value + 1);

  if (shlvl == NULL) {
    setenv("SHLVL", "1", 1);
    return;
  }

  setenv("SHLVL", shlvl, 1);
  free(shlvl);
}
