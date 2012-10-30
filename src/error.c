#include "error.h"
#include "asprintf.h"

#include <stdarg.h>

static const char *type_strings[] = { "FATAL", "WARNING", "NOTICE", NULL };
extern char *prog;

#define __log(s, args...)  \
    fprintf(stderr, s, ##args); /* freopen specific (stderr) */ \
    fprintf(stdout, s, ##args); /* on console */

void __noreturn error_nret(const char *str, ...)
{
    va_list ap;
    char *buff;

    va_start(ap, str);
    vasprintf(&buff, str, ap); 
    va_end(ap);

    __log(buff);
    __log("%s: error is not recoverable, terminating now...\n", prog);
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

    if (error_type == LOG_NULL) {       /* special type for logging */    
        __log("%s: %s", prog, buff);
    } else {
        __log("%s: %s: %s", prog, type_strings[error_type], buff);
    }
}

