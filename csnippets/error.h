#ifndef __error_h
#define __error_h

#define LOG_NULL     -1
/*
 * WARNING: Do not change these numbers as they're used as `index'
 * for `type_strings' see error.c fore more information.
 */
#define LOG_FATAL     0
#define LOG_WARNING   1
#define LOG_NOTICE    2

#define LOG_FILE "stderr.log"    /* log file to print to.  */
/* shortcuts. */
#define log(str, args...)       error(LOG_NOTICE, str, ##args)
#define die fatal
#define fatal(str, args...)     error_nret(str, ##args)
#define warning(str, args...)   error(LOG_WARNING, str, ##args)
#define print(str, args...)     error(LOG_NULL, str, ##args)

/** Initialize log file */
#define log_init()          freopen(LOG_FILE, "w", stderr);

/**
 * Log a formatted/non-formatted string to stderr and stdout.
 *
 * NOTE: This function doesn't return, that means the program will
 * exit after.
 */
extern void __noreturn error_nret(const char *str, ...);
/**
 * Log errno and the string (strerror).
 *
 * This function doesn't return (exits after).
 */
extern void __noreturn log_errno(const char *str, ...);
/**
 * Log a string.
 *
 * This function doesn't exit.
 */
extern void error(int log_type, const char *str, ...);

#endif  /* __error_h */

