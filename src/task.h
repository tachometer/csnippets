#ifndef __task_h
#define __task_h

#include "list.h"

__begin_header

typedef void (*task_routine) (void *);

typedef struct {
    task_routine start_routine;
    void *param;
    struct list_node node;
} task_t;

extern void tasks_init(void);
extern void tasks_stop(void);
extern void tasks_add(task_t *task);

extern task_t *task_create(task_routine routine, void *param);

__end_header

#endif  /* __task_h */

