#include "lib/error.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/attrib.h"

#define UNW_LOCAL_ONLY
#include <libunwind.h>

void error(const char *str) {
  int res = fputs(str, stderr);

  assert(res != EOF);
}

ATTRIB_PRINTF(1, 2)
void error_f(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  int res = vfprintf(stderr, format, ap);
  va_end(ap);

  assert(res != -1);
}

ATTRIB_NORETURN
void fatal(const char *str) {
  int res = fputs(str, stderr);

  assert(res != EOF);

  exit(EXIT_FAILURE);
}

ATTRIB_NORETURN
ATTRIB_PRINTF(1, 2)
void fatal_f(const char *restrict format, ...) {
  va_list ap;
  va_start(ap, format);

  int res = vfprintf(stderr, format, ap);
  va_end(ap);

  assert(res != -1);

  exit(EXIT_FAILURE);
}

void rash_unwind(void) {
  unw_cursor_t cursor;
  unw_context_t context;

  // Initialize cursor to current frame for local unwinding.
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  // Unwind frames one by one, going up the frame stack.
  while (unw_step(&cursor) > 0) {
    unw_word_t offset, pc;
    unw_get_reg(&cursor, UNW_REG_IP, &pc);
    if (pc == 0) {
      break;
    }
    printf("0x%lx:", pc);

    char sym[256];
    if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
      printf(" (%s+0x%lx)\n", sym, offset);
    } else {
      printf(" -- error: unable to obtain symbol name for this frame\n");
    }
  }
}
