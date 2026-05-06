/*
 * The code in this file is absolutely diabolical and has outlived many
 * refactors because I don't want to touch it. Maybe I'll refactor it
 * eventually, but today is not that day...
 */

#include "auto_complete.h"

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
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
#include "line_reader/types.h"

#ifndef _DIRENT_HAVE_D_TYPE
  #include <sys/stat.h>
#endif

#include "builtins/find_builtin.h"
#include "lib/buffer.h"
#include "lib/string.h"
#include "lib/vector.h"

static String tilde_expand(const String *path) {
  if (path->length == 0) {
    return (String){0};
  }

  if (path->data[0] != '~') {
    return string_clone(path);
  }

  size_t first_slash = string_index_of(path, '/');

  if (first_slash == (size_t)-1) {
    first_slash = path->length;
  }

  // path == "~/..."
  if (first_slash == 1) {
    const char *home = getenv("HOME");

    if (home == NULL) {
      return string_clone(path);
    }

    String result = string_from_cstr(home);
    string_append_ptr(&result, path->data + 1, path->length - 1);

    return result;
  }

  String username = string_substring(path, 1, first_slash);

  struct passwd *pw = getpwnam(string_cstr(&username));

  string_destroy(&username);

  if (pw == NULL || pw->pw_dir == NULL) {
    return string_clone(path);
  }

  String result = string_from_cstr(pw->pw_dir);
  string_append_ptr(
      &result, path->data + first_slash, path->length - first_slash
  );

  return result;
}

static void split_path(const String *path, String *dirname, String *basename) {
  size_t last_slash = string_last_index_of(path, '/');

  if (last_slash == (size_t)-1) {
    *dirname = string_from_cstr(".");
    *basename = string_clone(path);
  } else {
    if (last_slash == 0) {
      *dirname = string_from_cstr("/");
    } else {
      *dirname = string_substring(path, 0, last_slash);
    }

    *basename = string_substring(path, last_slash + 1, path->length);
  }
}

static void match_file(StringList *matches, const String *word) {
  String dirname, basename;
  DIR *dir;

  split_path(word, &dirname, &basename);

  {
    String expanded = tilde_expand(&dirname);
    dir = opendir(string_cstr(&expanded));
    string_destroy(&expanded);
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
    char *file = ent->d_name;

    // must match hidden files explicitly
    if (file[0] == '.') {
      if (basename.length < 1 || basename.data[0] != '.') {
        continue;
      }
    }

    if (strncmp(file, basename.data, basename.length) != 0) {
      continue;
    }

    String full_path = string_clone(&dirname);
    string_append_char(&full_path, '/');
    string_append_cstr(&full_path, ent->d_name);

#ifdef _DIRENT_HAVE_D_TYPE
    if (ent->d_type == DT_DIR) {
      string_append_char(&full_path, '/');
    } else {
      string_append_char(&full_path, ' ');
    }
#else
    {
      struct stat sb = {0};
      if (fstatat(fd, ent->d_name, &sb, AT_SYMLINK_NOFOLLOW) == 0 &&
          S_ISDIR(sb.st_mode)) {
        string_append_char(&full_path, '/');
      } else {
        string_append_char(&full_path, ' ');
      }
    }
#endif

    VECTOR_PUSH(*matches, full_path);
  }

  string_destroy(&dirname);
  string_destroy(&basename);
  closedir(dir);
}

static void match_exec_file(StringList *matches, const String *word) {
  String dirname, basename;

  split_path(word, &dirname, &basename);

  DIR *dir = opendir(string_cstr(&dirname));

  if (dir == NULL) {
    return;
  }

  int fd = dirfd(dir);
  assert(fd != -1);
  struct dirent *ent;

  while ((ent = readdir(dir))) {
    char *file = ent->d_name;

    // must match hidden files explicitly
    if (file[0] == '.') {
      if (basename.length < 1 || basename.data[0] != '.') {
        continue;
      }
    }

    if (strncmp(file, basename.data, basename.length) != 0) {
      continue;
    }

    // if file is not executable
    if (faccessat(fd, file, X_OK, AT_EACCESS) != 0) {
      continue;
    }

    String full_path = string_clone(&dirname);
    string_append_char(&full_path, '/');
    string_append_cstr(&full_path, ent->d_name);

#ifdef _DIRENT_HAVE_D_TYPE
    if (ent->d_type == DT_DIR) {
      string_append_char(&full_path, '/');
    } else {
      string_append_char(&full_path, ' ');
    }
#else
    {
      struct stat sb = {0};
      if (fstatat(fd, ent->d_name, &sb, AT_SYMLINK_NOFOLLOW) == 0 &&
          S_ISDIR(sb.st_mode)) {
        string_append_char(&full_path, '/');
      } else {
        string_append_char(&full_path, ' ');
      }
    }
#endif

    VECTOR_PUSH(*matches, full_path);
  }

  string_destroy(&dirname);
  string_destroy(&basename);
  closedir(dir);
}

static void match_command(StringList *matches, const String *word) {
  find_matching_builtins(word, matches);
  char *path = getenv("PATH");

  if (path == NULL) {
    return;
  }

  path = strdup(path);

  char *path_part = strtok(path, ":");
  while (path_part != NULL) {
    DIR *dir = opendir(path_part);

    if (dir == NULL) {
      continue;
    }

    int fd = dirfd(dir);
    assert(fd != -1);
    struct dirent *ent;

    while ((ent = readdir(dir))) {
      char *file = ent->d_name;

      // must match hidden files explicitly
      if (file[0] == '.' && word->data[0] != '.') {
        continue;
      }

      if (strncmp(file, word->data, word->length) != 0) {
        continue;
      }

      // if file is not executable
      if (faccessat(fd, file, X_OK, AT_EACCESS) != 0) {
        continue;
      }

      String command = string_from_cstr(file);
      string_append_char(&command, ' ');

      VECTOR_PUSH(*matches, command);
    }

    closedir(dir);
    path_part = strtok(NULL, ":");
  }

  free(path);
}

static void pretty_print_strings(const StringList *strings) {
  size_t width = (size_t)get_terminal_width();
  size_t max_len = 2;

  size_t num_printed = 0;
  for (; num_printed < strings->length; num_printed++) {
    const size_t new_len = strings->data[num_printed].length + 2;

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
    printf("%-*s", (int)max_len, string_cstr(&strings->data[i]));

    if ((i + 1) % col == 0) {
      printf("\n");
    }
  }

  if (num_printed != strings->length) {
    printf("...");
  }
}

typedef enum {
  DEFAULT,
  WHITESPACE,
  DOUBLE_QUOTE,
  SINGLE_QUOTE
} WordState;

static String
find_last_word(LineReader *reader, bool *is_first_word, WordState *end_state) {
  WordState state = WHITESPACE;
  unsigned word_count = 0;

  uint8_t *source = reader->active_buffer->data;
  size_t word_start = 0;

  size_t i;
  for (i = 0; i < reader->buffer_offset; i++) {
    const uint8_t curr = source[i];

    switch (state) {
      case DEFAULT:
        if (curr == '"') {
          state = DOUBLE_QUOTE;
          break;
        }

        if (curr == '\'') {
          state = SINGLE_QUOTE;
          break;
        }

        if (isspace((int)curr)) {
          state = WHITESPACE;
          break;
        }

        break;

      case WHITESPACE:
        if (isspace((int)curr)) {
          break;
        }

        word_count++;

        state = DEFAULT;
        word_start = i;
        i--;
        break;

      case DOUBLE_QUOTE:
        if (curr == '"') {
          state = DEFAULT;
          break;
        }

        break;

      case SINGLE_QUOTE:
        if (curr == '\'') {
          state = DEFAULT;
          break;
        }

        break;
    }
  }

  *is_first_word = (word_count == 1);
  *end_state = state;

  return string_using_ptr(
      (char *)(word_start + reader->active_buffer->data),
      reader->buffer_offset - word_start
  );
}

ATTRIB_UNUSED
static size_t common_prefix_len(const StringList *strings, size_t begin) {
  for (size_t i = begin;; i++) {
    for (size_t j = 0; j < strings->length - 1; j++) {
      if (i >= strings->data[j].length || i >= strings->data[j + 1].length) {
        return i;
      }
      if (strings->data[j].data[i] != strings->data[j + 1].data[i]) {
        return i;
      }
    }
  }
}

void auto_complete(LineReader *reader) {
  // cannot auto complete nothing
  if (reader->buffer_offset == 0) {
    return;
  }

  bool is_first_word;
  WordState end_state;
  String word = find_last_word(reader, &is_first_word, &end_state);

  StringList matches = {0};

  if (is_first_word) {
    if (string_contains(&word, '/')) {
      match_exec_file(&matches, &word);
    } else {
      match_command(&matches, &word);
    }
  } else {
    match_file(&matches, &word);
  }

  if (matches.length == 0) {
    VECTOR_DESTROY(matches);
    return;
  }

  size_t bytes_written = 0;

  if (matches.length == 1) {
    String *match = matches.data;

    if (match->length > word.length) {
      bytes_written = match->length - word.length;

      copy_hist_buf_if_needed(reader);

      buffer_insert_bulk(
          reader->active_buffer,
          (uint8_t *)match->data + word.length,
          bytes_written,
          reader->buffer_offset
      );
    }
  } else {
    // find how many characters all the matches have in common
    size_t i = common_prefix_len(&matches, word.length);

    if (i > word.length) {
      bytes_written = i - word.length;

      copy_hist_buf_if_needed(reader);

      buffer_insert_bulk(
          reader->active_buffer,
          (uint8_t *)matches.data[0].data + word.length,
          bytes_written,
          reader->buffer_offset
      );
    } else {
      sort_strings(&matches);
      putchar('\n');
      pretty_print_strings(&matches);
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
      FLUSH();
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

  stringlist_destroy(&matches);
}
