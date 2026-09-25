#include "interpreter/repl.h"
#include "rash.h"
#include "global.h"

Rash instance;

int main(int argc, char **argv) {
  RashInstanceInitResult result = rash_instance_init(&instance, argc, argv);

  if (result == UNEXPECTED_FAILURE) {
    return 1;
  }

  if (result == EXPECTED_FAILURE) {
    return 0;
  }

  repl(&instance.reader);

  rash_instance_delete(&instance);

  return 1;
}
