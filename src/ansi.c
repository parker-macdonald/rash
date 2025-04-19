#include "ansi.h"

#include <stdint.h>

const char ANSI_START_CHAR = '\033';
const char ASCII_END_OF_TRANSMISSION = '\04';
const char ASCII_DEL = '\177';
const uint8_t RECV_SIGINT = 0xff;

const char ANSI_CURSOR_LEFT[] = "\033[D";
const char ANSI_CURSOR_RIGHT[] = "\033[C";

const char ANSI_REMOVE_FULL_LINE[] = "\033[2K";

const char ANSI_CURSOR_POS_SAVE[] = "\033[s";
const char ANSI_CURSOR_POS_RESTORE[] = "\033[u";
