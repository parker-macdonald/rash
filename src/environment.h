#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

/**
 * @brief set an environment variable given a key and a value. in case it isn't
 * obvious, the data passed in is cloned.
 * @param key the key of the environment variable (think KEY=VALUE).
 * @param value the value of the environment variable (think KEY=VALUE).
 * @return returns 1 if the key is invalid (contains an '=' character or is
 * empty, posix forbids this). 0 is returned otherwise.
 */
int env_set(const char *const key, const char *const value);

/**
 * @brief set an environment variable given a key value pair (KEY=VALUE). in
 * case it isn't obvious, the data passed in is cloned.
 * @param env_string the string used for the environment variable (KEY=VALUE).
 * @return returns 1 if the string is invalid (starts with an '=' character or
 * is empty, posix forbids this). 0 is returned otherwise.
 */
int env_put(const char *const env_string);

/**
 * @brief get an environment variable given a key.
 * @param key the key of the variable
 * @return returns the associated value or null
 */
const char *env_get(const char *const key);

/**
 * @brief unset an environment variable given a key
 * @param key the key of the variable
 * @return returns 1 if no variable was removed, returns 0 otherwise.
 */
int env_unset(const char *const key);

/**
 * @brief fill the internal variable structure with the contents of environ
 */
void env_init(void);

/**
 * @brief return a char** that can be used as an environ for execve.
 */
char **env_get_environ(void);

/**
 * @brief prints all variables in a list.
 */
void env_print(void);

#endif
