#ifndef __util_h
#define __util_h

#ifndef __cplusplus
#ifdef _MSC_VER
typedef enum {
    false = 0,
    true  = 1
} bool;
#else
#include <stdbool.h>
#endif
#endif
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "linux_style.h"
#include "error.h"

/*
 * countof() returns the count of array, i.e
 * const char *my_array[] = { ... };
 * int count = countof(my_array);
 */
#define countof(array) (sizeof((array)) / sizeof((array)[0]))
/*
 * some helpers for easy memory management.
 */
#define xfree(p) do { if (p) free(p); p = NULL; } while (0)
#define __alloc_failure(s) do { warning("failed to allocate %zd bytes\n", s); } while(0)
#define xmalloc(p, s, action) do {  \
    p = malloc(s); \
    if (unlikely(!p)) { \
        __alloc_failure(s); \
        action; \
    } \
} while (0)
#define xcalloc(p, l, s, action) do { \
    p = calloc(l, s); \
    if (unlikely(!p)) { \
        __alloc_failure(s * l); \
        action; \
    } \
} while (0)
#define xrealloc(new_ptr, old_ptr, count, action) do { \
    new_ptr = realloc(old_ptr, count);  \
    if (unlikely(!new_ptr)) {\
        __alloc_failure(count); \
        action; \
    } \
} while (0)
#ifndef min
#define min(a, b)                         \
    __extension__ ({                      \
        __typeof__(a) _a = (a);           \
        __typeof__(b) _b = (b);           \
        _a < _b ? _a : _b;                \
    })
#endif
#ifndef max
#define max(a, b)                         \
    __extension__ ({                      \
        __typeof__(a) _a = (a);           \
        __typeof__(b) _b = (b);           \
        _a > _b ? _a : _b;                \
    })
#endif

#ifdef __cplusplus
#define __begin_header extern "C" {
#define __end_header   }
#else
#define __begin_header
#define __end_header
#endif

#ifdef _WIN32
#ifndef __use_select
    #define __use_select
#endif
#ifdef __use_epoll
    #undef __use_epoll
#endif
#ifdef __use_kqueue
    #undef __use_kqueue
#endif
#define ERRNO WSAGetLastError()
#define set_last_error(e) SetLastError((e))
#define E_BLOCK WSAEWOULDBLOCK
#define E_AGAIN EAGAIN
#define E_ISCONN WSAEISCONN
#define E_ALREADY WSAEALREADY
#define E_INPROGRESS WSAEINPROGRESS
#else
#ifdef __use_kqueue
    #error "kqueue isn't implemented yet."
#endif
#if !defined __use_epoll && !defined __use_select
    #define __use_epoll
#endif
#define ERRNO errno
#define set_last_error(e) errno = (e)
#define E_BLOCK EWOULDBLOCK
#define E_AGAIN EAGAIN
#define E_ISCONN EISCONN
#define E_ALREADY EALREADY
#define E_INPROGRESS EINPROGRESS
#endif

#endif  /* __util_h */

