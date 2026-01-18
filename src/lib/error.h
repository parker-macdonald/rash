#ifndef F_ERROR_H

void error(const char* str);

__attribute__((__format__(__printf__, 1, 2)))
void error_f(const char *restrict format, ...);

void fatal(const char* str);

__attribute__((__format__(__printf__, 1, 2)))
void fatal_f(const char *restrict format, ...);

#endif
