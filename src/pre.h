#ifdef __pre_h
#error "PCH shouldn't be included more than once!"
#endif

#define __pre_h
#ifndef __cplusplus
#include <stdbool.h>
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
#include "platform.h"

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
#define __fail(s) do { warning("failed to allocate %zd bytes\n", s); } while(0)
#define xmalloc(p, s, action) do {  \
    p = malloc(s); \
    if (unlikely(!p)) { \
        __fail(s); \
        action; \
    } \
} while (0)
#define xcalloc(p, l, s, action) do { \
    p = calloc(l, s); \
    if (unlikely(!p)) { \
        __fail(s); \
        action; \
    } \
} while (0)
#define xrealloc(new_ptr, old_ptr, count, action) do { \
    new_ptr = realloc(old_ptr, count);  \
    if (unlikely(!new_ptr)) \
        action; \
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

