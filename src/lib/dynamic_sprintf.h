#ifndef DYNAMIC_SPRINTF_H
#define DYNAMIC_SPRINTF_H

__attribute__((__format__(__printf__, 1, 2)))
char *dynamic_sprintf(const char *restrict format, ...);

#endif
