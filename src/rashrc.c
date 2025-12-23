#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_reader.h"
#include "interpreter/repl.h"

int load_rashrc(void) {
  const char *home = getenv("HOME");

  if (home == NULL) {
    return 1;
  }

  size_t len = strlen(home);
  if (len + sizeof("/.rashrc") > PATH_MAX) {
    return 1;
  }

  char rc_path[PATH_MAX];

  memcpy(rc_path, home, len);
  strcpy(rc_path + len, "/.rashrc");

  FILE *rashrc = fopen(rc_path, "r");
  if (rashrc == NULL) {
    // if opening the rashrc fails for a reason other than the file does not
    // exist, i.e. file permission error, or the value of home is malformed.
    if (errno != ENOENT) {
      perror("Failed to load .rashrc file");
    }

    return 1;
  }

  struct file_reader rashrc_reader;
  file_reader_init(&rashrc_reader, rashrc);

  repl((const uint8_t *(*)(void *))file_reader_read, &rashrc_reader);

  return 0;
}
