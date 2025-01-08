#ifndef ENVIROMENT_H
#define ENVIROMENT_H

void env_init(void);

void env_add(const char *const env_str);

void env_remove(const char *const env_str);

char **env_get_array(void);

char* env_get(const char *const env_name);

void env_destroy(void);

#endif