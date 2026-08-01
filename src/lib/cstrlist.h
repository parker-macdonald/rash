#ifndef CSTRLIST_H
#define CSTRLIST_H

#include "lib/vector.h"

typedef VECTOR(char *) CStrList;

void cstr_list_destroy(CStrList *list);

#endif
