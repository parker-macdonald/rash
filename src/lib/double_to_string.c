#include "lib/double_to_string.h"

#include "lib/buffer.h"
#include "lib/dynamic_sprintf.h"
#include "lib/parse.h"
#include <math.h>

Buffer double_to_string(double x) {
  if (isnan(x)) {
    return buffer_from_cstr("NaN");
  }
  if (!isfinite(x)) {
    if (x > 0) {
      return buffer_from_cstr("Infinity");
    }
    return buffer_from_cstr("-Infinity");
  }

  int trailing_digits = 0;

  char *buffer;
  while (1) {
    buffer = dynamic_sprintf("%.*f", trailing_digits, x);
    OptionDouble parsed = parse_double(buffer);

    // this can happen on very large numbers, strtod doesn't accept them
    if (!parsed.has_value) {
      break;
    }

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
