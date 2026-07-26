#include "auto_complete.h"

#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "lib/utf_8.h"
#include "lib/buffer.h"
#include "lib/vector.h"
#include "line_reader/draw.h"
#include "line_reader/types.h"
#include "builtins/find_builtin.h"
#include "line_reader/action_utils.h"

static Buffer expand_path(const Buffer *path) {
  if (buffer_starts_with_byte(path, '~')) {
    const char *home = getenv("HOME");

    if (home == NULL) {
      // lol
      return buffer_clone(path);
    }

    Buffer full_path = buffer_clone(path);

    // remove '~'
    buffer_remove_n(&full_path, 0, 1);

    buffer_insert_cstr(&full_path, 0, home);

    return full_path;
  }

  return buffer_clone(path);
}

static DIR *open_parent_dir(const Buffer *path, Buffer *basename) {
  DIR *dir;
  Buffer expanded_path = expand_path(path);

  size_t last_slash = buffer_find_last(&expanded_path, '/');

  // no slash means to search the current directory
  if (last_slash == (size_t)-1) {
    dir = opendir(".");
    *basename = buffer_clone(&expanded_path);
  } else {
    Buffer dirname = buffer_slice(&expanded_path, 0, last_slash);

    dir = opendir(buffer_cstr(&dirname));

    buffer_destroy(&dirname);

    // using last_slash + 1 is ok because if last_slash is the last character
    // of path, the resulting buffer will be empty
    // i.e.
    // path = "example_path/"
    // basename = buffer_slice(word, 13, 13)
    // basename = ""
    *basename = buffer_slice(&expanded_path, last_slash + 1, expanded_path.length);
  }

  buffer_destroy(&expanded_path);

  return dir;
}

static void match_file(BufferList *matches, const Buffer *word) {
  Buffer basename;
  DIR *dir = open_parent_dir(word, &basename);

  size_t last_slash = buffer_find_last(word, '/');

  // cant open directory, nothing we can do
  if (dir == NULL) {
    buffer_destroy(&basename);
    return;
  }

  int fd = dirfd(dir);
  assert(fd != -1);

  struct dirent *ent;

  while ((ent = readdir(dir))) {
    Buffer filename = buffer_from_cstr(ent->d_name);

    // must match dot files explicitly
    // basename can have a length of zero, so checking if basename.u8_ptr[0] == '.'
    // is unsafe. that's why i'm using buffer_starts_with instead
    if (ent->d_name[0] == '.' && !buffer_starts_with_byte(&basename, '.')) {
      buffer_destroy(&filename);
      continue;
    }

    if (!buffer_starts_with_buffer(&filename, &basename)) {
      buffer_destroy(&filename);
      continue;
    }

    struct stat sb = {0};
    if (
      fstatat(
        fd,
        ent->d_name,
        &sb,
        AT_SYMLINK_NOFOLLOW
      ) == 0 &&
      S_ISDIR(sb.st_mode)
    ) {
      // true if filepath is a directory
      buffer_append_byte(&filename, '/');
    } else {
      // true if filepath is not a directory (i.e. regular file or sym link)
      buffer_append_byte(&filename, ' ');
    }

    // insert path to be beginning of filename
    if (last_slash != (size_t)-1) {
      buffer_insert_ptr(
        &filename,
        0,
        word->void_ptr,
        last_slash + 1
      );
    }

    VECTOR_PUSH(*matches, filename);
  }

  buffer_destroy(&basename);
  closedir(dir);
}

// TODO: maybe find a way to make this not just a copy paste of match_file with
// minor changes? code duplication bad
static void match_exe_file(BufferList *matches, const Buffer *word) {
  Buffer basename;
  DIR *dir = open_parent_dir(word, &basename);

  size_t last_slash = buffer_find_last(word, '/');

  // cant open directory, nothing we can do
  if (dir == NULL) {
    buffer_destroy(&basename);
    return;
  }

  int fd = dirfd(dir);
  assert(fd != -1);

  struct dirent *ent;

  while ((ent = readdir(dir))) {
    Buffer filename = buffer_from_cstr(ent->d_name);

    // must match dot files explicitly
    // basename can have a length of zero, so checking if basename.u8_ptr[0] == '.'
    // is unsafe. that's why i'm using buffer_starts_with instead
    if (ent->d_name[0] == '.' && !buffer_starts_with_byte(&basename, '.')) {
      buffer_destroy(&filename);
      continue;
    }

    if (!buffer_starts_with_buffer(&filename, &basename)) {
      buffer_destroy(&filename);
      continue;
    }

    struct stat sb = {0};
    // if fstatat fails, or file is not a directory, or file is not executable
    if (fstatat(fd, ent->d_name, &sb, AT_SYMLINK_NOFOLLOW) != 0) {
      buffer_destroy(&filename);
      continue;
    }

    // if is directory
    if (S_ISDIR(sb.st_mode)) {
      buffer_append_byte(&filename, '/');
    }
    // if is executable
    else if (sb.st_mode & S_IXUSR) {
      buffer_append_byte(&filename, ' ');
    }
    // fail if file is not a directory or executable
    else {
      buffer_destroy(&filename);
      continue;
    }


    // insert path to be beginning of filename
    if (last_slash != (size_t)-1) {
      buffer_insert_ptr(
        &filename,
        0,
        word->void_ptr,
        last_slash + 1
      );
    }

    VECTOR_PUSH(*matches, filename);
  }

  buffer_destroy(&basename);
  closedir(dir);
}

static void match_command(BufferList *matches, const Buffer *word) {
  find_matching_builtins(word, matches);

  char *path_env;

  {
    const char *path_cstr = getenv("PATH");
  
    if (path_cstr == NULL) {
      return;
    }

    path_env = strdup(path_cstr);
  }

  char *path = strtok(path_env, ":");

  while (path != NULL) {
    DIR *dir = opendir(path);

    if (dir != NULL) {
      int fd = dirfd(dir);
      assert(fd != -1);
      
      struct dirent *ent;
      while ((ent = readdir(dir))) {
        Buffer filename = buffer_from_cstr(ent->d_name);

        // must match dot files explicitly
        // basename can have a length of zero, so checking if basename.u8_ptr[0] == '.'
        // is unsafe. that's why i'm using buffer_starts_with instead
        if (ent->d_name[0] == '.' && !buffer_starts_with_byte(word, '.')) {
          buffer_destroy(&filename);
          continue;
        }

        if (!buffer_starts_with_buffer(&filename, word)) {
          buffer_destroy(&filename);
          continue;
        }

        // if file is not executable
        if (faccessat(fd, ent->d_name, X_OK, AT_EACCESS) != 0) {
          buffer_destroy(&filename);
          continue;
        }

        buffer_append_byte(&filename, ' ');

        VECTOR_PUSH(*matches, filename);
      }

      closedir(dir);
    }

    path = strtok(NULL, ":");
  }

  free(path_env);
}

static void pretty_print_strings(const BufferList *list) {
  size_t width = (size_t)get_terminal_width();
  size_t max_len = 2;

  size_t num_printed = 0;
  for (; num_printed < list->length; num_printed++) {
    const size_t new_len = list->data[num_printed].length + 2;

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
    Buffer string = buffer_clone(list->data + i);

    printf("%-*s", (int)max_len, buffer_cstr(&string));

    buffer_destroy(&string);

    if ((i + 1) % col == 0) {
      printf("\r\n");
    }
  }

  if (num_printed != list->length) {
    printf("...");
  }

  printf("\r\n");
}

static void reader_insert_bulk(LineReader *reader, const Buffer *to_insert) {
  copy_hist_buf_if_needed(reader);

  buffer_insert_buffer(
    reader->active_buffer,
    reader->buffer_offset,
    to_insert
  );

  const unsigned n = utf8_count_codepoint(to_insert);

  reader->buffer_offset += to_insert->length;
  reader->cursor_pos += n;

  draw_entire_state(reader);
  draw_flush();
}

void auto_complete(LineReader *reader) {
  // cannot auto complete nothing
  if (reader->buffer_offset == 0) {
    return;
  }

  size_t word_start = buffer_find_prev(
    reader->active_buffer,
    ' ',
    reader->buffer_offset
  );

  // word starts after the last space. this also works if buffer_find_prev
  // returns -1 since if there are no spaces we want word_start to be 0
  word_start++;

  Buffer word = buffer_slice(
    reader->active_buffer,
    word_start,
    reader->buffer_offset
  );

  // this can happen if the user presses tab following a space
  if (word.length == 0) {
    // buffer_destroy isn't necessary because buffers of zero length have no
    // associated memory, but that could hypothetically change in the future
    // (likely because of a regression), and it also doesnt hurt anything to
    // have it here
    buffer_destroy(&word);
    return;
  }

  BufferList matches = {0};

  bool is_first_word;
  
  {
    size_t first_space = buffer_find_first(reader->active_buffer, ' ');

    if (first_space == (size_t)-1) {
      first_space = 0;
    }

    is_first_word = first_space == word_start;
  }

  if (is_first_word) {
    if (buffer_contains_byte(&word, '/')) {
      match_exe_file(&matches, &word);
    } else {
      match_command(&matches, &word);
    }
  } else {
    match_file(&matches, &word);
  }

  // cant match anything = nothing to do
  if (matches.length == 0) {
    buffer_list_destroy(&matches);
    return;
  }

  if (matches.length == 1) {
    Buffer *match = matches.data;

    // this happens when there's one match that's the same as the word
    // i.e.
    // matches = ["hello"]
    // word = "hello"
    if (match->length == word.length) {
      buffer_list_destroy(&matches);
      return;
    }

    copy_hist_buf_if_needed(reader);

    Buffer to_insert = buffer_slice(match, word.length, match->length);

    reader_insert_bulk(reader, &to_insert);

    buffer_destroy(&to_insert);
    buffer_list_destroy(&matches);

    return;
  }

  // code below this comment runs when matches.length > 1

  Buffer prefix = buffer_list_longest_common_prefix(&matches);

  // this happens with the word matches multiple things, but the largest common
  // prefix of the matches is the word.
  // i.e.
  // matches = ["abcd", "abba", "aboba"]
  // word = "ab"
  if (prefix.length == word.length) {
    buffer_list_sort(&matches);

    draw_cursor_post_line(reader);
    pretty_print_strings(&matches);
    draw_entire_state(reader);

    draw_flush();

    buffer_destroy(&prefix);
    buffer_list_destroy(&matches);
    return;
  }

  Buffer to_insert = buffer_slice(&prefix, word.length, prefix.length);
  buffer_destroy(&prefix);
      
  reader_insert_bulk(reader, &to_insert);

  buffer_destroy(&to_insert);
  buffer_list_destroy(&matches);
}
