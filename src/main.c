#include "interpreter/repl.h"
#include "rash.h"
#include "rashrc.h"

int main(int argc, char **argv) {
  Rash rash = rash_instance_init(argc, argv);

  rash_register_global_instance(&rash);

  if (rash.interactive) {
    load_rashrc();
  }

  repl(&rash.reader);

  rash_instance_delete(&rash);

  return 1;
}
