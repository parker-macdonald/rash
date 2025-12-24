#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "../vec_types.h"
#include "modify_line.h"

size_t auto_complete(buf_t *line, size_t cursor_pos) {
  size_t word_start = cursor_pos - 1;
  size_t last_slash = (size_t)-1;

  for (; word_start > 0; word_start--) {
    if (last_slash == (size_t)-1 && line->data[word_start] == '/') {
      last_slash = word_start;
    }

    if (line->data[word_start] == ' ') {
      word_start++;
      break;
    }
  }

  DIR *dir;
  char *basename;
  size_t basename_len;

  // relative path
  if (last_slash == (size_t)-1) {
    dir = opendir(".");
    basename = (char *)(line->data + word_start);
    basename_len = cursor_pos - word_start;
  } else {
    // absolute path
    size_t path_len = last_slash - word_start;

    if (path_len == 0) {
      return 0;
    }

    char *path = malloc(path_len + 1);
    if (path == NULL) {
      return 0;
    }

    memcpy(path, line->data + word_start, path_len);
    path[path_len] = '\0';

    dir = opendir(path);

    free(path);

    basename = (char *)(line->data + last_slash + 1);
    basename_len = cursor_pos - last_slash - 1;
  }

  if (dir == NULL) {
    return 0;
  }

  struct dirent *ent;
  VECTOR(char *) matches;
  VECTOR_INIT(matches, 0);

  while ((ent = readdir(dir))) {
    // must match hidden files explicitly
    if (ent->d_name[0] == '.' && basename[0] != '.') {
      continue;
    }
    if (strncmp(ent->d_name, basename, basename_len) == 0) {
      VECTOR_PUSH(matches, strdup(ent->d_name));
    }
  }

  if (matches.length == 0) {
    closedir(dir);
    return 0;
  }

  if (matches.length == 1) {
    size_t match_len = strlen(matches.data[0]);
    size_t bytes_written = match_len - basename_len;

    int fd = dirfd(dir);
    if (fd != -1) {
      struct stat sb = {0};
      if (fstatat(fd, matches.data[0], &sb, 0) != -1) {
        matches.data[0][match_len] = S_ISDIR(sb.st_mode) ? '/' : ' ';
        bytes_written++;
      }
    } else {
      // if someone fails to run dirfd, they're on some crazy exotic *nix
      // kernel
      perror(
          "if you get this error, your running some pretty exotic "
          "configuration.\ndirfd"
      );
    }

    line_insert_bulk(
        line,
        cursor_pos,
        (uint8_t *)matches.data[0] + basename_len,
        bytes_written
    );

    free(matches.data[0]);
    VECTOR_DESTROY(matches);
    closedir(dir);
    return bytes_written;
  }

  closedir(dir);

  size_t i;

  for (i = 0;; i++) {
    for (size_t j = 0; j < matches.length - 1; j++) {
      if (matches.data[j][i] != matches.data[j + 1][i]) {
        goto leave;
      }
    }
  }

leave: {
  size_t bytes_written = i - basename_len;
  if (bytes_written != 0) {
    line_insert_bulk(
        line,
        cursor_pos,
        (uint8_t *)matches.data[0] + basename_len,
        bytes_written
    );
  }

  for (size_t j = 0; j < matches.length; j++) {
    free(matches.data[j]);
  }
  VECTOR_DESTROY(matches);
  return bytes_written;
}
}
