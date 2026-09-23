#ifndef FILE_READER_H
#define FILE_READER_H

#include "readers/generic_reader.h"

int file_reader_create(GenericReader *out, const char *filepath);

#endif
