#include "search_path.h"

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "lib/buffer.h"
#include "vector.h"

char *search_path(const char *file) {
  const char *path = getenv("PATH");
  if (path == NULL) {
    // musl does this same thing, not sure why /sbin or /usr/sbin aren't there.
    path = "/usr/local/bin:/bin:/usr/bin";
  }

  Buffer file_path = buffer_create(16);

  for (size_t i = 0;; i++) {
    if (path[i] == ':' || path[i] == '\0') {
      assert(file_path.length != 0);

      buffer_append_byte(&file_path, '/');

      buffer_append_cstr(&file_path, file);

      char *cstr = buffer_cstr(&file_path);

      if (faccessat(AT_FDCWD, cstr, X_OK, AT_EACCESS) == 0) {
        return cstr;
      }

      if (path[i] == '\0') {
        break;
      }

      VECTOR_CLEAR(file_path);
      continue;
    }

    buffer_append_char(&file_path, path[i]);
  }

  buffer_destroy(&file_path);
  return NULL;
}
