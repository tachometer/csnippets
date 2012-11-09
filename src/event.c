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
#include "event.h"

#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

static pthread_mutex_t mutex        = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond         = PTHREAD_COND_INITIALIZER;
static bool            running = false;
static pthread_t       self;
static LIST_HEAD(events);

static void *events_thread(void *d)
{
    event_t *event = NULL;
    struct timespec ts;
    struct timeval tv;

    while (running) {
        pthread_mutex_lock(&mutex);
        if (list_empty(&events)) {
            pthread_cond_wait(&cond, &mutex);
            if (!running)
                break;
        } else {
            event = list_top(&events, event_t, node);
            list_del(&event->node);

            gettimeofday(&tv, NULL);
            ts.tv_sec  = tv.tv_sec;
            ts.tv_nsec = tv.tv_usec * 1000;
            ts.tv_sec += event->delay;

            pthread_cond_timedwait(&cond, &mutex, &ts);
        }
        pthread_mutex_unlock(&mutex);

        if (!event)
            continue;

        tasks_add(event->task);
        free(event);
    }

    /* if we have any remaining events, add them to tasks  */
    while (!list_empty(&events)) {
        event = list_top(&events, event_t, node);
        if (!event)
            break;
        list_del(&event->node);

        tasks_add(event->task);
        free(event);
    }
    return NULL;
}

void events_init(void)
{
    pthread_attr_t attr;
    int rc;

    rc = pthread_attr_init(&attr);
    if (rc != 0)
        fatal("failed to initialize thread attributes\n");

    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (rc != 0)
        fatal("failed to setdetachstate");

    rc = pthread_create(&self, &attr, &events_thread, NULL);
    if (rc != 0)
        fatal("failed to create thread");
    running = true;
}

void events_stop(void)
{
    pthread_mutex_lock(&mutex);
    running = false;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_join(self, NULL);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

event_t *event_create(int delay, task_routine start, void *p)
{
    event_t *event;
    if (!start || delay < 0)
        return NULL;
    event = malloc(sizeof(event_t));
    if (!event)
        return NULL;
    event->delay = delay;
    event->task = task_create(start, p);
    if (!event->task) {
        free(event);
        return NULL;
    }
    return event;
}

void events_add(event_t *event)
{
    bool empty = false;

    if (!event)
        return;
    pthread_mutex_lock(&mutex);
    if (running) {
        empty = list_empty(&events);
        list_add(&events, &event->node);
    } else
        printf("attempting to add an event to a terminated event queue\n");

    pthread_mutex_unlock(&mutex);
    if (empty)
        pthread_cond_signal(&cond);
}

