#include <stdint.h>
#include <stdio.h>
#include <sys/utsname.h>

#include "../vector.h"

unsigned int print_prompt(const char *const prompt) {
  unsigned int characters_printed = 0;
  VECTOR(uint8_t) data;
  VECTOR_INIT(data);

  for (size_t i = 0; prompt[i] != '\0'; i++) {
    if (prompt[i] == '\\') {
      if (prompt[i + 1] == '\0') {
        VECTOR_PUSH(data, '\\');
        characters_printed++;
        break;
      }

      switch (prompt[i + 1]) {
        case 'e':
          VECTOR_PUSH(data, '\033');
          i += 2;
          break;
        case 'h': {
          struct utsname name;
          (void)uname(&name);
          for (size_t j = 0;
               name.nodename[j] != '.' && name.nodename[j] != '\0';
               j++) {
            VECTOR_PUSH(data, name.nodename[j]);
            characters_printed++;
          }
          i += 2;
          break;
        }

        case 'H': {
          struct utsname name;
          (void)uname(&name);
          for (size_t j = 0; name.nodename[j] != '\0'; j++) {
            VECTOR_PUSH(data, name.nodename[j]);
            characters_printed++;
          }
          i += 2;
          break;
        }
      }
    }

    VECTOR_PUSH(data, prompt[i]);
    characters_printed++;
  }

  fwrite(data.data, data.length, 1, stdout);

  return characters_printed;
}
