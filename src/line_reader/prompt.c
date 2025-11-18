#include "prompt.h"

#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "../vector.h"

static char *getcwd_good(void) {
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

prompts_t get_prompts(const char *const prompt) {
  prompts_t prompts;
  VECTOR_INIT(prompts);

  unsigned int characters = 0;
  VECTOR(char) buffer;
  VECTOR_INIT(buffer);

  unsigned int increment = 1;

  for (size_t i = 0; prompt[i] != '\0'; i++) {
    if (prompt[i] == '\\') {
      if (prompt[i + 1] == '\0') {
        VECTOR_PUSH(buffer, '\\');
        characters += increment;
        break;
      }

      switch (prompt[i + 1]) {
        case 'e':
          VECTOR_PUSH(buffer, '\033');
          i++;
          continue;

        case 'h': {
          struct utsname name;
          (void)uname(&name);
          for (size_t j = 0;
               name.nodename[j] != '.' && name.nodename[j] != '\0';
               j++) {
            VECTOR_PUSH(buffer, name.nodename[j]);
            characters += increment;
          }
          i++;
          continue;
        }

        case 'H': {
          struct utsname name;
          (void)uname(&name);
          for (size_t j = 0; name.nodename[j] != '\0'; j++) {
            VECTOR_PUSH(buffer, name.nodename[j]);
            characters += increment;
          }
          i++;
          continue;
        }

        case 'n':
          VECTOR_PUSH(buffer, '\n');
          characters += increment;
          i++;
          continue;

        case 'r':
          VECTOR_PUSH(buffer, '\r');
          characters += increment;
          i++;
          continue;

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
            characters += increment;

            for (; cwd[j] != '\0'; j++) {
              VECTOR_PUSH(buffer, cwd[j]);
              characters += increment;
            }

            i++;
            continue;
          }

        just_copy_all:
          for (size_t j = 0; cwd[j] != '\0'; j++) {
            VECTOR_PUSH(buffer, cwd[j]);
            characters += increment;
          }

          i++;
          continue;
        }

        case 'u': {
          struct passwd *p = getpwuid(geteuid());

          if (p == NULL) {
            break;
          }

          for (size_t j = 0; p->pw_name[j] != '\0'; j++) {
            VECTOR_PUSH(buffer, p->pw_name[j]);
            characters += increment;
          }

          i++;
          continue;
        }

        case '$':
          if (geteuid() == 0) {
            VECTOR_PUSH(buffer, '#');
            characters += increment;
          } else {
            VECTOR_PUSH(buffer, '$');
            characters += increment;
          }
          i++;
          continue;

        case '[':
          increment = 0;
          i++;
          continue;

        case ']':
          increment = 1;
          i++;
          continue;

        case ',':
          if (buffer.length == 0) {
            VECTOR_PUSH(
                prompts,
                ((struct prompt){.data = NULL, .length = 0, .characters = 0})
            );
            continue;
          }

          VECTOR_PUSH(
              prompts,
              ((struct prompt){.data = buffer.data,
                               .length = buffer.length,
                               .characters = characters})
          );
          VECTOR_INIT(buffer);
          increment = 1;
          characters = 0;
          i++;
          continue;
      }
    }

    VECTOR_PUSH(buffer, prompt[i]);
    characters += increment;
  }

  if (buffer.length == 0) {
    VECTOR_DESTROY(buffer);
    VECTOR_PUSH(
        prompts, ((struct prompt){.data = NULL, .length = 0, .characters = 0})
    );
  } else {
    VECTOR_PUSH(
        prompts,
        ((struct prompt){.data = buffer.data,
                         .length = buffer.length,
                         .characters = characters})
    );
  }

  return prompts;
}
