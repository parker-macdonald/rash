#include "raw_mode.h"

#include <termios.h>
#include <unistd.h>

static struct termios oldt = {0};

void enable_raw_mode(void) {
  // she turned me into a newt!
  struct termios newt = {0};

  tcgetattr(STDIN_FILENO, &oldt);

  newt = oldt;

  /*
    from the man page (cfmakeraw() is not in posix so im not using it):
    Raw mode
       cfmakeraw() sets the terminal to something like the "raw" mode of
       the old Version 7 terminal driver: input is available character by
       character, echoing is disabled, and all special processing of
       terminal input and output characters is disabled.  The terminal
       attributes are set as follows:
  */
  newt.c_iflag = ~(tcflag_t)(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                             ICRNL | IXON);
  newt.c_oflag &= ~(tcflag_t)OPOST;
  newt.c_lflag &= ~(tcflag_t)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  newt.c_cflag &= ~(tcflag_t)(CSIZE | PARENB);
  newt.c_cflag |= CS8;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void disable_raw_mode(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}
