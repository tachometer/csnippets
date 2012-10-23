#ifdef __pre_h
#error "PCH shouldn't be included more than once!"
#endif

#define __pre_h
#include "linux_style.h"
#include "error.h"

/*
 * countof() returns the count of array, i.e
 * const char *my_array[] = { ... };
 * int count = countof(my_array);
 */
#define countof(array) (sizeof(array) / sizeof(__typeof__(array)))
/*
 * some helpers for easy memory management.
 */
#define xfree(p) do { free(p); p = NULL; } while (0)
#define __fail(s) do { fprintf(stderr, "failed to allocate %zd bytes\n", s); } while(0)
#define xmalloc(p, s, action) do {  \
    p = malloc(sizeof s); \
    if (p == NULL) { \
        __fail(sizeof s); \
        action; \
    } \
} while (0)
#define xcalloc(p, l, s, action) do { \
    p = calloc(l, sizeof s); \
    if (p == NULL) { \
        __fail(sizeof s); \
        action; \
    } \
} while (0)

#define min(a, b)                         \
    __extension__ ({                      \
        __typeof__(a) _a = (a);           \
        __typeof__(b) _b = (b);           \
        _a < _b ? _a : _b;                \
    })

#define max(a, b)                         \
    __extension__ ({                      \
        __typeof__(a) _a = (a);           \
        __typeof__(b) _b = (b);           \
        _a > _b ? _a : _b;                \
    })
