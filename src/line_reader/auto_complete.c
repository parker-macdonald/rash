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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/ansi.h"
#include "lib/attrib.h"
#include "lib/is_ident.h"
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

static void match_file(StringList *matches, String *word) {
  DIR *dir;
  String basename;
  size_t last_slash = string_last_index_of(word, '/');

  // relative path
  if (last_slash == (size_t)-1) {
    dir = opendir(".");
    basename = string_clone(word);
  } else {
    String path;

    if (last_slash == 0) {
      path = string_from_cstr("/");
    } else {
      path = string_substring(word, 0, last_slash);
    }

    dir = opendir(string_cstr(&path));

    string_destroy(&path);

    basename = string_substring(word, last_slash + 1, word->length);
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
    if (file[0] == '.' && basename.data[0] != '.') {
      continue;
    }

    if (strncmp(file, basename.data, basename.length) != 0) {
      continue;
    }

    String full_path = string_substring(word, 0, last_slash + 1);

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

    VECTOR_PUSH(*matches, string_cstr(&full_path));
  }

  closedir(dir);
}

static void match_command(StringList *matches, String *word) {
  find_matching_builtins(word, matches);
  const char *path = getenv("PATH");

  if (path == NULL) {
    return;
  }

  String file_path = {0};

  for (size_t i = 0; path[i] != '\0'; i++) {
    char *
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

typedef enum {
  DEFAULT,
  WHITESPACE,
  DOUBLE_QUOTE,
  SINGLE_QUOTE,
  SINGLE_LITERAL,
  ENV_NO_BRACE,
  ENV_BRACE,
  VAR_EXPANSION,
  TILDE,
  SUBSHELL,
} WordState;

String find_last_word(LineReader *reader, bool *is_first_word, WordState *end_state) {
  WordState state = WHITESPACE;
  unsigned word_count = 0;

  uint8_t *source = reader->active_buffer->data;
  size_t word_start = 0;

  size_t i;
  for (i = 0; i < reader->buffer_offset; i++) {
    const uint8_t curr = source[i];
    const uint8_t next = source[i + 1];

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

      if (curr == '\\') {
        state = SINGLE_LITERAL;
        break;
      }

      if (isspace((int)curr)) {
        state = WHITESPACE;
        break;
      }

      if (curr == '$') {
        switch (next) {
        case '(':
          state = SUBSHELL;
          word_start = i + 2;
          break;
        case '{':
          state = ENV_BRACE;
          word_start = i + 2;
          break;
        default:
          if (is_ident(next)) {
            word_start = i + 1;
            state = ENV_NO_BRACE;
            break;
          }
          state = DEFAULT;
          break;
        }
      }

      if (curr == '{') {
        state = VAR_EXPANSION;
        word_start = i + 1;
        break;
      }

      // comments
      if (curr == '#' && next == ' ') {
        goto leave;
      }

      break;

    case WHITESPACE:
      if (isspace((int)curr)) {
        break;
      }

      word_count++;

      if (curr == '~') {
        state = TILDE;
        word_start = i + 1;
        break;
      }

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

    case SINGLE_LITERAL:
      state = DEFAULT;
      break;

    case ENV_NO_BRACE:
      if (!is_ident(curr)) {
        state = DEFAULT;
        break;
      }

      break;

    case ENV_BRACE:
      if (curr == '}') {
        state = DEFAULT;
        break;
      }

      break;

    case SUBSHELL:
      if (curr == ')') {
        state = DEFAULT;
        break;
      }

      break;

    case VAR_EXPANSION:
      if (curr == '}') {
        state = DEFAULT;
        break;
      }

      break;

    case TILDE:
      if (curr == '/') {
        state = DEFAULT;
        break;
      }

      break;
    }
  }

  *is_first_word = (word_count == 1);
  *end_state = state;

  return string_using_ptr((char *)(word_start + reader->active_buffer->data), reader->buffer_offset - word_start);

leave:
  return string_using_ptr(NULL, 0);
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
      match_exec_file()
    } else {
      match_command(&matches, &word);
    }
  } else {
    match_file(&matches, word);
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

      buffer_insert_bulk(reader->active_buffer,
                         (uint8_t *)matches.data[0] + word_len, bytes_written,
                         reader->buffer_offset);
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

      buffer_insert_bulk(reader->active_buffer,
                         (uint8_t *)matches.data[0] + word_len, bytes_written,
                         reader->buffer_offset);
    } else {
      sort_strings(&matches);
      putchar('\n');
      pretty_print_strings(matches.data, matches.length);
      printf("\n" ANSI_CURSOR_POS_SAVE "%s%.*s" ANSI_CURSOR_POS_RESTORE,
             reader->prompt, (int)reader->active_buffer->length,
             (char *)reader->active_buffer->data);

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
        reader->active_buffer->data + reader->buffer_offset, bytes_written);

    const unsigned n = utf8_count_codepoint(&buffer);
    cursor_right_n(reader, n);

    PUTS(ANSI_CURSOR_POS_SAVE);

    draw_active_buffer(reader);

    PUTS(ANSI_CURSOR_POS_RESTORE);

    FLUSH();

    reader->buffer_offset += bytes_written;
  }
}
