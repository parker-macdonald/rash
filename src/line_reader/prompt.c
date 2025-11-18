#include <errno.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "../vector.h"

char *getcwd_good(void) {
  size_t cwd_size = 16;
  char *cwd = malloc(sizeof(char) * cwd_size);

  char *buf = getcwd(cwd, cwd_size);

  while (buf == NULL) {
    if (errno != ERANGE) {
      free(cwd);
      return NULL;
    }

    cwd_size *= 2;

    char *temp_cwd = realloc(cwd, cwd_size);

    if (temp_cwd == NULL) {
      free(cwd);
      return NULL;
    } else {
      cwd = temp_cwd;
    }

    buf = getcwd(cwd, cwd_size);
  }

  return cwd;
}

unsigned int print_prompt(const char *const prompt) {
  unsigned int characters_printed = 0;
  VECTOR(uint8_t) buffer;
  VECTOR_INIT(buffer);

  unsigned int counting = 1;

  for (size_t i = 0; prompt[i] != '\0'; i++) {
    if (prompt[i] == '\\') {
      if (prompt[i + 1] == '\0') {
        VECTOR_PUSH(buffer, '\\');
        characters_printed += counting;
        break;
      }

      switch (prompt[i + 1]) {
        case 'e':
          VECTOR_PUSH(buffer, '\033');
          i++;
          break;

        case 'h': {
          struct utsname name;
          (void)uname(&name);
          for (size_t j = 0;
               name.nodename[j] != '.' && name.nodename[j] != '\0';
               j++) {
            VECTOR_PUSH(buffer, name.nodename[j]);
            characters_printed += counting;
          }
          i++;
          break;
        }

        case 'H': {
          struct utsname name;
          (void)uname(&name);
          for (size_t j = 0; name.nodename[j] != '\0'; j++) {
            VECTOR_PUSH(buffer, name.nodename[j]);
            characters_printed += counting;
          }
          i++;
          break;
        }

        case 'n':
          VECTOR_PUSH(buffer, '\n');
          characters_printed += counting;
          i++;
          break;

        case 'r':
          VECTOR_PUSH(buffer, '\r');
          characters_printed += counting;
          i++;
          break;

        case 'w': {
          char *cwd = getcwd_good();
          if (cwd == NULL) {
            break;
          }

          char *home = getenv("HOME");
          if (home != NULL) {
            size_t j;
            for (j = 0; home[j] != '\0' && cwd[j] != '\0'; j++) {
              if (cwd[j] != home[j]) {
                goto just_copy_all;
              }
            }

            VECTOR_PUSH(buffer, '~');
            characters_printed += counting;

            for (; cwd[j] != '\0'; j++) {
              VECTOR_PUSH(buffer, cwd[j]);
              characters_printed += counting;
            }

            i++;
            break;
          }

        just_copy_all:
          for (size_t j = 0; cwd[j] != '\0'; j++) {
            VECTOR_PUSH(buffer, cwd[j]);
            characters_printed += counting;
          }

          i++;
          break;
        }

        case 'u': {
          struct passwd *p = getpwuid(geteuid());

          if (p == NULL) {
            break;
          }

          for (size_t j = 0; p->pw_name[j] != '\0'; j++) {
            VECTOR_PUSH(buffer, p->pw_name[j]);
            characters_printed += counting;
          }

          i++;
          break;
        }

        case '$':
          if (geteuid() == 0) {
            VECTOR_PUSH(buffer, '#');
            characters_printed += counting;
          } else {
            VECTOR_PUSH(buffer, '$');
            characters_printed += counting;
          }
          i++;
          break;

        case '[':
          counting = 0;
          i++;
          break;

        case ']':
          counting = 1;
          i++;
          break;
      }

      continue;
    }

    VECTOR_PUSH(buffer, prompt[i]);
    characters_printed += counting;
  }

  fwrite(buffer.data, buffer.length, 1, stdout);

  return characters_printed;
}
