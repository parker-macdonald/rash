#ifndef GLOB_H
#define GLOB_H

#include "../vector.h"
#include "../vec_types.h"

extern char *glob_err_msg;

strings_t *glob(const char *pattern);

#endif
