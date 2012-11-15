/*
 * Copyright (c) 2012 asamy <f.fallen45@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __event_h
#define __event_h

#include "task.h"

__begin_header

typedef struct {
    int32_t delay;           /* Delay in seconds, the thread will wait before executing this.  */
    task_t *task;            /* The task pointer, Internal use only.
                                This is created by the thread.  */
    struct list_node node;   /* Next/Prev event.  */
} event_t;

/**
 * Initialize events thread
 *
 * Creates a thread that runs indepdently and can be stopped
 * via calling events_stop().
 */
extern void events_init(void);
/**
 * Stop events thread, this adds any running event to the task queue.
 *
 */
extern void events_stop(void);
/**
 * Add an event to the event list.
 *
 * @param event, create it with event_create().
 */
extern void events_add(event_t *event);
/**
 * Create an event, NOTE: This does NOT add it to the list.
 * You must add it manually via events_add().
 *
 * Example usage:
 *    events_add(event_create(10, my_func, my_param));
 */
extern event_t *event_create(int delay, task_routine start, void *p);

__end_header
#endif  /* __event_h */

