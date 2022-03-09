/*

This file complements @config.h to include Visual Studio and Windows's
specific configurations.

When upgrading the project, copy a new config.h.generic from the /src
directory, rename to config.h and edit that file, including this header
there.

#include "visualstudioconfig.h"

And you're done.

These other variables must be defined within the project configurations, 
because it may vary depending on target compilation:

PCRE2_STATIC			  -- MUST be set for static libraries, also for the executables who
							 are using pcre2.h and are linking to the static libraries (or else, when
							 you try to compile your code, PCRE functions will get redefined and you'll lose
							 the symbols).
SUPPORT_UNICODE			  -- CAN be set for binaries compiled in UNICODE mode.
PCRE2_CODE_UNIT_WIDTH	  -- MUST be set to 8, 16 or 32 on the library project or else it won't compile.
HAVE_CONFIG_H             -- MUST be set globally on the library project.


All files on this solution uses the name conventions defined on the official document here:
https://www.pcre.org/current/doc/html/pcre2build.html#SEC3

We reserved the original names to DLL files and added a "static" to static link libraries.

*/

#pragma once

/* Define to 1 if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */
#define HAVE_INTTYPES_H 1
/* Define to 1 if you have the <limits.h> header file. */
/* #undef HAVE_LIMITS_H */
#define HAVE_LIMITS_H 1
/* Define to 1 if you have the `memmove' function. */
/* #undef HAVE_MEMMOVE */
#define HAVE_MEMMOVE 1
/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */
#define HAVE_STDINT_H 1
/* Define to 1 if you have the <stdio.h> header file. */
/* #undef HAVE_STDIO_H */
#define HAVE_STDIO_H 1
/* Define to 1 if you have the <stdlib.h> header file. */
/* #undef HAVE_STDLIB_H */
#define HAVE_STDLIB_H 1
/* Define to 1 if you have the `strerror' function. */
/* #undef HAVE_STRERROR */
#define HAVE_STRERROR 1
/* Define to 1 if you have the <sys/stat.h> header file. */
/* #undef HAVE_SYS_STAT_H */
#define HAVE_SYS_STAT_H 1
/* Define to 1 if you have the <sys/types.h> header file. */
/* #undef HAVE_SYS_TYPES_H */
#define HAVE_SYS_TYPES_H 1
/* Define to 1 if you have the <wchar.h> header file. */
/* #undef HAVE_WCHAR_H */
#define HAVE_WCHAR_H 1
/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */
#define HAVE_WINDOWS_H 1

/* Define to any value to enable support for Just-In-Time compiling. */
/* #undef SUPPORT_JIT */
#define SUPPORT_JIT 

// Comments on JIT: To be able to compile it, we must disable a severe warnings (Level 2)
// that are now treated as error by Visual Studio. This is C4146. This is done by defining the
// SDL checks on compilator to No (command line /sld-). All projects here are already set to that.
// https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4146
#ifdef SUPPORT_JIT 
#pragma warning (disable : 4146)
#endif

// Also we disabled some warnings here because we don't want to change code:
#pragma warning (disable : 4244 4267)

/* Define to any value to enable the 16 bit PCRE2 library. */
/* #undef SUPPORT_PCRE2_16 */
#define SUPPORT_PCRE2_16 1
/* Define to any value to enable the 32 bit PCRE2 library. */
/* #undef SUPPORT_PCRE2_32 */
#define SUPPORT_PCRE2_32 1
/* Define to any value to enable the 8 bit PCRE2 library. */
/* #undef SUPPORT_PCRE2_8 */
#define SUPPORT_PCRE2_8 1

/* Define to any value to enable support for Unicode and UTF encoding. This
   will work even in an EBCDIC environment, but it is incompatible with the
   EBCDIC macro. That is, PCRE2 can support *either* EBCDIC code *or*
   ASCII/Unicode, but not both at once. */
   /* #undef SUPPORT_UNICODE */
// Comment: This is better defined on the project settings.
// All projects here use UNICODE.

// Define this flag if you're planning on not changing deprecated functions.
// Since all our projects here aren't touching the sources...
#define _CRT_SECURE_NO_WARNINGS   