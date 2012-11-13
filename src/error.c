#include "error.h"
#include "asprintf.h"

#include <stdarg.h>

static const char *type_strings[] = { "fatal error", "warning", "notice", NULL };
extern char *prog;

#define __log(s, args...) do { \
    fprintf(stderr, s, ##args); /* freopen specific (stderr) */ \
    fflush(stderr);             /* write the changes right away */ \
    fprintf(stdout, s, ##args); /* on console */ \
} while (0)

void __noreturn error_nret(const char *str, ...)
{
    va_list ap;
    char *buff;

    va_start(ap, str);
    vasprintf(&buff, str, ap); 
    va_end(ap);

    __log("%s: %s\n%s: error is not recoverable, terminating now...\n",
            prog, buff, prog);
    exit(EXIT_FAILURE);
}

void __noreturn log_errno(const char *str, ...)
{
    va_list ap;
    char *buff;

    va_start(ap, str);
    (void) vasprintf(&buff, str, ap);
    va_end(ap);

    __log("%s: %s (%d): %s\n", prog, buff, errno, strerror(errno));
    __log("%s: error is not recoverable, terminating now...\n", prog);
    exit(EXIT_FAILURE);
}

void error(int error_type, const char *str, ...)
{
    va_list va;
    char *buff;

    va_start(va, str);
    (void) vasprintf(&buff, str, va);
    va_end(va);

    assert((error_type == -1 || error_type < countof(type_strings)));
    if (error_type == LOG_NULL) /* special type for logging */
        __log("%s: %s", prog, buff);
    else
        __log("%s: %s: %s", prog, type_strings[error_type], buff);
}

