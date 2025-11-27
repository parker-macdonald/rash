#ifndef SHELL_VARS_H
#define SHELL_VARS_H

void set_var(const char *const key, const char *const value);

const char *get_var(const char *const key);

// returns 1 if no var was deleted or 0
int unset_var(const char *const key);

#endif
