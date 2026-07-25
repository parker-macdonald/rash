#include "version.h"

const char *const VERSION_STRING = "rash version: "
#ifdef VERSION
    VERSION
#else
                                   "no one knows"
#endif
                                   "\r\nreleased on: "
#ifdef RELEASED
    RELEASED
#else
                                   "who knows when"
#endif
    ;
