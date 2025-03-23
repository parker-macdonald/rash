#ifndef FIND_CMD_H
#define FIND_CMD_H

typedef int (*builtin_t)(char **);

void trie_init(void);

void trie_destroy(void);

builtin_t find_builtin(const char *const str);

#endif
