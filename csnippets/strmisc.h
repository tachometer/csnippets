#ifndef __str_misc_h
#define __str_misc_h

extern char *strtrim(char *str);
extern char **strexplode(char *string, char seperator, int *size);
extern int strwildmatch(const char *pattern, const char *string);

extern bool str_cmp(const char *str, int (*cmp_func) (int));
/**
 * returns a malloc'd converted version of \a str
 */
extern char *str_convert(const char *str, int (*convert_func) (int));

#endif /*  __str_misc_h */

