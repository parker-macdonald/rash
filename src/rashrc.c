#include "rashrc.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "file_reader.h"
#include "interpreter/repl.h"
#include "lib/dynamic_sprintf.h"

int load_rashrc(void) {
  const char *home = getenv("HOME");

  if (home == NULL) {
    return 1;
  }

  char *rc_path = dynamic_sprintf("%s/.rashrc", home);

  FILE *rashrc = fopen(rc_path, "r");

  free(rc_path);

  if (rashrc == NULL) {
    // if opening the rashrc fails for a reason other than the file does not
    // exist, i.e. file permission error, or the value of home is malformed.
    if (errno != ENOENT) {
      perror("Failed to load .rashrc file");
    }

    return 1;
  }

  FileReader rashrc_reader;
  file_reader_init(&rashrc_reader, rashrc);

  repl(file_reader_read_void, &rashrc_reader);

  return 0;
}
