/* 
 * Copyright (C) 2012  asamy <f.fallen45@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __event_h
#define __event_h

#include "task.h"

__begin_header

typedef struct {
    int32_t delay;
    task_t *task;
    struct list_node node;
} event_t;

extern void events_init(void);
extern void events_stop(void);
extern void events_add(event_t *event);

extern event_t *event_create(int delay, task_routine start, void *p);

__end_header
#endif  /* __event_h */

