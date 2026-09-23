#include "generic_reader.h"
#include "readers/interactive_reader/interactive_reader.h"

const Buffer *generic_reader_read(GenericReader *self) {
  return self->read_line(self->reader_data);
}

void generic_reader_destroy(GenericReader *self) {
  self->destroy(self->reader_data);
}

static const Buffer *interactive_reader_read_void(void *reader_ptr) {
  return interactive_reader_read((InteractiveReader *)reader_ptr);
}

static void interactive_reader_destroy_void(void *reader_ptr) {
  interactive_reader_destroy((InteractiveReader *)reader_ptr);
}

void generic_reader_from_interactive(GenericReader *out, InteractiveReader *interactive) {
  out->reader_data = interactive;
  out->destroy = interactive_reader_destroy_void;
  out->read_line = interactive_reader_read_void;
}