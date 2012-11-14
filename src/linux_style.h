/**
 * Various linux stlye macros, when I say linux style,
 * I mean the macro style used in the Linux kernel (__noreturn, __weak, ...)
 *
 * Defines shortcuts for __attribute__ if __GNUC__ (GNU C compiler used.)
 */
#ifndef __linux_style_h
#define __linux_style_h

#ifdef __GNUC__
#define __noreturn      __attribute__((noreturn))
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define __inline__      inline __attribute__((always_inline))
#define __warn_unused   __attribute__((warn_unused_result))
#define __weak          __attribute__((weak))
#define __printf(nfmt, narg) \
    __attribute((format(__printf__, nfmt, narg)))
#else
#define __attribute__(x)
#define __noreturn
#define likely(x) (x)
#define unlikely(x) (x)
#define __inline__ inline
#define __warn_unused
#define __weak
#define __printf(nfmt, narg)
#endif

#endif    /* __linux_style_h */

