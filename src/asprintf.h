/*
 * This code is licensed under BSD-MIT By Rusty Russell, see also:
 * https://github.com/rustyrussell/ccan/
 */

#ifndef __asprintf_h
#define __asprintf_h

#include <stdarg.h>

__begin_header

extern int __printf(2, 3) asprintf(char **strp, const char *fmt, ...);
extern int vasprintf(char **strp, const char *fmt, va_list va);

__end_header

#endif  /* __asprintf_h */

