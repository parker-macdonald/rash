#include "shlvl.h"

#include <errno.h>
#include <stdlib.h>

#include "lib/dynamic_sprintf.h"

void set_shlvl(void) {
  char *shlvl = getenv("SHLVL");

  if (shlvl == NULL) {
    setenv("SHLVL", "1", 1);
    return;
  }

  char *endptr;
  errno = 0;
  unsigned long shlvl_num = strtoul(shlvl, &endptr, 10);

  if (errno != 0 || *endptr != '\0') {
    setenv("SHLVL", "1", 1);
    return;
  }

  shlvl = dynamic_sprintf("%lu", shlvl_num + 1);

  if (shlvl == NULL) {
    setenv("SHLVL", "1", 1);
    return;
  }

  setenv("SHLVL", shlvl, 1);
  free(shlvl);
}
