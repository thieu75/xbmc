/* config.h.  Generated by hand.  */
#ifdef _XBOX
#include <xtl.h>
#elif defined(_LINUX)
#include "PlatformInclude.h"
#else
#include <windows.h>
#endif
#include <stdio.h>

//#define HAVE_DLFCN_H 1
#define HAVE_DVDCSS_DVDCSS_H 1
/* #undef HAVE_DVDCSS_DVDCSS_H*/
/* #undef HAVE_INTTYPES_H */
#define HAVE_MEMORY_H 1
/* #undef HAVE_STDINT_H */
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
/* #undef HAVE_UNISTD_H */
#define PACKAGE "libdvdread"
#define PACKAGE_BUGREPORT ""
#define PACKAGE_NAME ""
#define PACKAGE_STRING ""
#define PACKAGE_TARNAME ""
#define PACKAGE_VERSION ""
#define STDC_HEADERS 1
#define VERSION "1.2.6"
/* #undef WORDS_BIGENDIAN */
/* #undef __DARWIN__ */
/* #undef const */
#define inline __inline
/* #undef size_t */

#define ssize_t __int64

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#define strcasecmp stricmp
#define strncasecmp strnicmp

#ifndef S_ISDIR
#define S_ISDIR(m) ((m) & _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) ((m) & _S_IFREG)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m) 0
#endif
#ifndef S_ISCHR
#define S_ISCHR(m) 0
#endif

/* Fallback types (very x86-centric, sorry) */
typedef unsigned char       uint8_t;
typedef signed char         int8_t;
typedef unsigned short      uint16_t;
typedef signed short        int16_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
typedef unsigned __int64    uint64_t;
typedef signed __int64      int64_t;
typedef unsigned int        uintptr_t;
