#ifndef FILE_READER_H
#define FILE_READER_H

#include <stdint.h>
#include <stdio.h>

void file_reader_init(FILE* fp);

const uint8_t* file_reader_read(void);

#endif
