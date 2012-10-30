#ifndef __config_h
#define __config_h

#include "list.h"

struct cdef_t {
    char key[33];
    char *value;
    struct list_node node;
    struct list_head def_children;
};

struct centry_t {
    char section[32];
    struct cdef_t *def;
    struct list_head children;
    struct list_node node;
};

extern struct centry_t *config_parse(const char *filename);

#endif   /* __config_h */

