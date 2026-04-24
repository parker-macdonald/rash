/*
 * The code in this file is absolutely diabolical and has outlived many
 * refactors because I don't want to touch it. Maybe I'll refactor it
 * eventually, but today is not that day...
 */

#include "auto_complete.h"

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/ansi.h"
#include "lib/attrib.h"
#include "lib/sort.h"
#include "lib/utf_8.h"
#include "line_reader/action_utils.h"
#include "line_reader/line_reader_struct.h"

#ifndef _DIRENT_HAVE_D_TYPE
  #include <sys/stat.h>
#endif

#include "builtins/find_builtin.h"
#include "lib/buffer.h"
#include "lib/string.h"
#include "lib/vector.h"

static void
get_file_matches(StringList *matches, const char *word, size_t word_len) {
  DIR *dir;
  const char *basename;
  size_t basename_len;
  size_t last_slash = (size_t)(-1);

  for (size_t i = 0; i < word_len; i++) {
    if (word[i] == '/') {
      last_slash = i;
    }
  }

  // relative path
  if (last_slash == (size_t)-1) {
    dir = opendir(".");
    basename = word;
    basename_len = word_len;
  } else {
    // absolute path
    size_t path_len = last_slash;

    if (path_len == 0) {
      path_len++;
    }

    char *path = malloc(path_len + 1);
    if (path == NULL) {
      return;
    }

    memcpy(path, word, path_len);
    path[path_len] = '\0';

    dir = opendir(path);

    free(path);

    basename = (const char *)(word + last_slash + 1);
    basename_len = word_len - last_slash - 1;
  }

  if (dir == NULL) {
    return;
  }

#ifndef _DIRENT_HAVE_D_TYPE
  int fd = dirfd(dir);
  assert(fd != -1);
#endif

  struct dirent *ent;

  while ((ent = readdir(dir))) {
    // must match hidden files explicitly
    if (ent->d_name[0] == '.' && basename[0] != '.') {
      continue;
    }
    if (strncmp(ent->d_name, basename, basename_len) != 0) {
      continue;
    }

    size_t file_len = strlen(ent->d_name);
    char *file_path = malloc(last_slash + file_len + 3);

    memcpy(file_path, word, last_slash + 1);
    memcpy(file_path + last_slash + 1, ent->d_name, file_len);

#ifdef _DIRENT_HAVE_D_TYPE
    if (ent->d_type == DT_DIR) {
      file_path[last_slash + file_len + 1] = '/';
    } else {
      file_path[last_slash + file_len + 1] = ' ';
    }
#else
    {
      struct stat sb = {0};
      if (fstatat(fd, ent->d_name, &sb, AT_SYMLINK_NOFOLLOW) == 0 &&
          S_ISDIR(sb.st_mode)) {
        file_path[last_slash + file_len + 1] = '/';
      } else {
        file_path[last_slash + file_len + 1] = ' ';
      }
    }
#endif

    file_path[last_slash + file_len + 2] = '\0';
    VECTOR_PUSH(*matches, file_path);
  }

  closedir(dir);
}

static void
get_command_matches(StringList *matches, const char *word, size_t word_len) {
  find_matching_builtins(word, word_len, matches);
  const char *path = getenv("PATH");

  if (path == NULL) {
    return;
  }

  String file_path;
  VECTOR_INIT(file_path);

  for (size_t i = 0; path[i] != '\0'; i++) {
    if (path[i] != ':' && path[i] != '\0') {
      VECTOR_PUSH(file_path, path[i]);
      continue;
    }
    VECTOR_PUSH(file_path, '\0');
    DIR *dir = opendir(file_path.data);

    if (dir == NULL) {
      continue;
    }

    int fd = dirfd(dir);
    assert(fd != -1);
    struct dirent *ent;

    while ((ent = readdir(dir))) {
      // must match hidden files explicitly
      if (ent->d_name[0] == '.' && word[0] != '.') {
        continue;
      }
      if (strncmp(ent->d_name, word, word_len) != 0) {
        continue;
      }
      // if file is not executable
      if (faccessat(fd, ent->d_name, X_OK, AT_EACCESS) != 0) {
        continue;
      }

      size_t file_len = strlen(ent->d_name);
      char *file = malloc(file_len + 2);

      memcpy(file, ent->d_name, file_len);
      file[file_len] = ' ';
      file[file_len + 1] = '\0';

      VECTOR_PUSH(*matches, file);
    }

    closedir(dir);

    VECTOR_CLEAR(file_path);
  }

  VECTOR_DESTROY(file_path);
}

ATTRIB_UNUSED
static size_t
get_matches(StringList *matches, Buffer *line, size_t cursor_pos) {
  size_t word_start = cursor_pos - 1;

  for (; word_start > 0; word_start--) {
    if (line->data[word_start] == ' ') {
      word_start++;
      break;
    }
  }

  char *word = (char *)line->data + word_start;
  size_t word_len = cursor_pos - word_start;

  if (word_len == 0) {
    return 0;
  }

  if (word_start == 0 && memchr(word, '/', word_len) == NULL) {
    get_command_matches(matches, word, word_len);
  } else {
    get_file_matches(matches, word, word_len);
  }

  return word_start;
}

static void pretty_print_strings(char *const strings[], const size_t length) {
  size_t width = (size_t)get_terminal_width();
  size_t max_len = 2;

  size_t num_printed = 0;
  for (; num_printed < length; num_printed++) {
    const size_t new_len = strlen(strings[num_printed]) + 2;

    if (new_len > max_len) {
      if ((num_printed + 2) * new_len / width > 3) {
        break;
      }
      max_len = new_len;
      continue;
    }

    if ((num_printed + 2) * max_len / width > 3) {
      break;
    }
  }

  size_t col = width / max_len;

  for (size_t i = 0; i < num_printed; i++) {
    printf("%-*s", (int)max_len, strings[i]);

    if ((i + 1) % col == 0) {
      printf("\n");
    }
  }

  if (num_printed != length) {
    printf("...");
  }
}

void auto_complete(LineReader *reader) {
  // cannot auto complete nothing
  if (reader->buffer_offset == 0) {
    return;
  }

  size_t word_start = reader->buffer_offset - 1;

  for (; word_start > 0; word_start--) {
    if (reader->active_buffer->data[word_start] == ' ') {
      word_start++;
      break;
    }
  }

  char *word = (char *)reader->active_buffer->data + word_start;
  size_t word_len = reader->buffer_offset - word_start;
  size_t bytes_written = 0;

  if (word_len == 0) {
    return;
  }

  StringList matches = {0};

  if (word_start == 0 && memchr(word, '/', word_len) == NULL) {
    get_command_matches(&matches, word, word_len);
  } else {
    get_file_matches(&matches, word, word_len);
  }

  if (matches.length == 0) {
    VECTOR_DESTROY(matches);
    return;
  }

  if (matches.length == 1) {
    size_t match_len = strlen(matches.data[0]);

    if (match_len > word_len) {
      bytes_written = match_len - word_len;

      copy_hist_buf_if_needed(reader);

      buffer_insert_bulk(
          reader->active_buffer,
          (uint8_t *)matches.data[0] + word_len,
          bytes_written,
          reader->buffer_offset
      );
    }

    free(matches.data[0]);
    VECTOR_DESTROY(matches);
  } else {
    size_t i;

    for (i = 0;; i++) {
      for (size_t j = 0; j < matches.length - 1; j++) {
        if (matches.data[j][i] != matches.data[j + 1][i]) {
          goto leave;
        }
        if (matches.data[j][i] == '\0' || matches.data[j + 1][i] == '\0') {
          goto leave;
        }
      }
    }

  leave: {
    if (i > word_len) {
      bytes_written = i - word_len;

      copy_hist_buf_if_needed(reader);

      buffer_insert_bulk(
          reader->active_buffer,
          (uint8_t *)matches.data[0] + word_len,
          bytes_written,
          reader->buffer_offset
      );
    } else {
      sort_strings(&matches);
      putchar('\n');
      pretty_print_strings(matches.data, matches.length);
      printf(
          "\n" ANSI_CURSOR_POS_SAVE "%s%.*s" ANSI_CURSOR_POS_RESTORE,
          reader->prompt,
          (int)reader->active_buffer->length,
          (char *)reader->active_buffer->data
      );

      unsigned short width = get_terminal_width();

      unsigned moves_down =
          ((reader->cursor_pos + reader->cursor_pos) / width) -
          (reader->cursor_pos / width);
      if (moves_down > 0) {
        printf(ANSI_CURSOR_DOWN_N("%u"), moves_down);
      }
      printf("\r" ANSI_CURSOR_RIGHT_N("%u"), reader->cursor_pos % width);
      (void)fflush(stdout);
    }

    for (size_t j = 0; j < matches.length; j++) {
      free(matches.data[j]);
    }
    VECTOR_DESTROY(matches);
  }
  }

  if (bytes_written) {
    Buffer buffer = buffer_using(
        reader->active_buffer->data + reader->buffer_offset, bytes_written
    );

    const unsigned n = utf8_count_codepoint(&buffer);
    cursor_right_n(reader, n);

    PUTS(ANSI_CURSOR_POS_SAVE);

    draw_active_buffer(reader);

    PUTS(ANSI_CURSOR_POS_RESTORE);

    FLUSH();

    reader->buffer_offset += bytes_written;
  }
}
