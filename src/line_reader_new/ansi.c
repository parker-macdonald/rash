#include "ansi.h"

#include <stdint.h>

const uint8_t ANSI_START_CHAR = '\033';
const uint8_t ASCII_END_OF_TRANSMISSION = '\04';
const uint8_t ASCII_DEL = '\177';

const char ANSI_CURSOR_LEFT[] = "\033[D";
const char ANSI_CURSOR_RIGHT[] = "\033[C";

const char ANSI_REMOVE_FULL_LINE[] = "\033[2K";
const char ANSI_REMOVE_BELOW_CURSOR[] = "\033[0J";

const char ANSI_CURSOR_POS_SAVE[] = "\033[s";
const char ANSI_CURSOR_POS_RESTORE[] = "\033[u";
