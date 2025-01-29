#include <stdio.h>

static FILE* history_file;

int history_int(void) {
    history_file = fopen("~/.rash_history", "a");

    if (history_file == NULL) {
        return 1;
    }

    return 0;
}
