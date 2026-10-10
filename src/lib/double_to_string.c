#include "lib/double_to_string.h"

#include "lib/buffer.h"
#include "lib/dynamic_sprintf.h"
#include "lib/error.h"
#include "lib/parse.h"

Buffer double_to_string(double x) {
  int trailing_digits = 0;

  char *buffer;
  while (1) {
    buffer = dynamic_sprintf("%.*f", trailing_digits, x);
    OptionDouble parsed = parse_double(buffer);
    rash_assert(parsed.has_value, "printed float cannot be converted back to float. this should be impossible");

    if (parsed.value == x) {
      break;
    }

    free(buffer);
    trailing_digits++;
  }
  
  Buffer result = buffer_from_cstr(buffer);
  free(buffer);

  return result;
}