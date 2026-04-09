#ifndef ATTRIB_H
#define ATTRIB_H

#if defined(__GNUC__)
  #define ATTRIB_PRINTF(arg_format, argv)                                      \
    __attribute__((__format__(__printf__, arg_format, argv)))
#else
  #define ATTRIB_PRINTF(arg_format, argv)
#endif

#if (__STDC_VERSION__ >= 202311L)
  #define ATTRIB_UNUSED [[maybe_unused]]
#elif defined(__GNUC__)
  #define ATTRIB_UNUSED __attribute__((unused))
#else
  #define ATTRIB_UNUSED
#endif

#if (__STDC_VERSION__ >= 202311L)
  #define ATTRIB_NORETURN [[noreturn]]
#else
  #define ATTRIB_NORETURN _Noreturn
#endif

#endif
