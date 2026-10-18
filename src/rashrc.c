#include "rashrc.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "readers/file_reader.h"
#include "interpreter/repl.h"
#include "lib/dynamic_sprintf.h"
#include "readers/generic_reader.h"

int load_rashrc(void) {
  const char *home = getenv("HOME");

  if (home == NULL) {
    return 1;
  }

  char *rc_path = dynamic_sprintf("%s/.rashrc", home);

  GenericReader reader;
  int status = file_reader_create(&reader, rc_path);
  free(rc_path);

  if (status) {
    // if opening the rashrc fails for a reason other than the file does not
    // exist, i.e. file permission error, or the value of home is malformed.
    if (errno != ENOENT) {
      perror("Failed to load .rashrc file");
    }

    return 1;
  }

  repl(&reader);

  generic_reader_destroy(&reader);

  return 0;
}
