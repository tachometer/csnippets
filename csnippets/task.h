#ifndef __task_h
#define __task_h

#include "list.h"

__begin_header

/** The start routine of a task.
 *  This will be called via the thread created by tasks_init().
 */
typedef void (*task_routine) (void *);

typedef struct {
    task_routine start_routine;    /* See above */
    void *param;                   /* The param to call the function (start_routine) with.  */
    struct list_node node;         /* The next and prev task */
} task_t;

/**
 * Initialize tasks thread.
 *
 * This is where all the tasks are executed.  The thread
 * runs indepdent (detached) but can be stopped via tasks_stop().
 *
 * if tasks_stop() is called when there are tasks still waiting,
 * the tasks will be executed before we exit.
 */
extern void tasks_init(void);
/**
 * Returns true if the tasks thread was initialized.
 */
extern bool tasks_running(void);
/**
 * Stops the tasks the thread, this means every waiting every
 * will be executed and memory will be free'd.
 */
extern void tasks_stop(void);
/**
 * Add a task to the task list.
 *
 * If the tasks thread was not initialized this function will throw
 * a warning on console, and will do nothing.
 */
extern void tasks_add(task_t *task);
/**
 * Create a task, NOTE: This does NOT add it to the queue.
 *
 * Returns a malloc'd task with task_routine routine and param param.
 * 
 * Add it with task_add(...);
 *
 * Example Usage:
 *     task_add(task_create(my_task, my_param));
 */
extern task_t *task_create(task_routine routine, void *param);

__end_header
#endif  /* __task_h */

