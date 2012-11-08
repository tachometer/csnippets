#include "strmisc.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *strtrim(char *str)
{
    char *end;

    /* trim leading spaces */
    while (isspace(*str)) str++;

    if (*str == 0)   /* all spaces? */
        return str;

    end = str + strlen(str) -1;
    while (end > str && isspace(*end)) end--;

    *(end+1) = '\0';
    return str;
}

char **strexplode(char *string, char separator, int *size)
{
    int start = 0, i, k = 0, count = 2;
    char **strarr;
    for (i = 0; string[i] != '\0'; i++)
        if (string[i] == separator)
            count++;

    *size = count-1;
    strarr = calloc(count, sizeof(char*));
    i = 0;
    while (*string != '\0') {
        if (*string == separator) {
            strarr[i] = calloc(k - start + 2,sizeof(char));
            strncpy(strarr[i], string - k + start, k - start);
            strarr[i][k - start + 1] = '\0'; /* ensure null termination */
            start = k;
            start++;
            i++;
        }
        string++;
        k++;
    }

    strarr[i] = calloc(k - start + 1,sizeof(char));
    strncpy(strarr[i], string - k + start, k - start);
    strarr[++i] = NULL;
 
    return strarr;
}

int strwildmatch(const char *pattern, const char *string) {
    switch (*pattern) {
        case '\0': return *string;
        case '*': return !(!strwildmatch(pattern+1, string) || (*string && !strwildmatch(pattern, string+1)));
        case '?': return !(*string && !strwildmatch(pattern+1, string+1));
        default: return !((toupper(*pattern) == toupper(*string)) && !strwildmatch(pattern+1, string+1));
    }
}

bool str_cmp(const char *str, int (*cmp_func) (int))
{
    char *p = (char *)str;
    if (!str || *str == '\0')
        return false;
    if (!cmp_func)
        return false;
    while (*p)
        /*
         * compare every single character in this string
         * if one fails, then the whole comparison fails...
         */
        if (!cmp_func((int)*p++))
            return false;
    return true;
}

char *str_convert(const char *str, int (*convert_func) (int))
{
    char *p;
    int len, i;

    if (!convert_func)
        return NULL;
    len = strlen(str);
    if (len < 0)
        return NULL;
    p = malloc(len + 1);
    if (!p)
        return NULL;
    for (i = 0; i < len; i++)
        p[i] = convert_func((int)str[i]);
    p[i + 1]  = '\0';
    return p;
}

