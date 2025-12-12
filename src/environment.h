#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

int env_set(const char *const key, const char *const value);

int env_put(const char *const env_string);

const char *env_get(const char *const key);

int env_unset(const char *const key);

void env_init(void);

char** env_get_environ(void);

#endif
