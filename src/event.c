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
static bool            running;
static pthread_t       self;
static LIST_HEAD(g_events);

static bool convert_time(struct timespec *ts, int delay)
{
    struct timeval tv;

    if (delay < 0)
        return false;

    gettimeofday(&tv, NULL);
    ts->tv_sec  = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
    ts->tv_sec += delay;

    return true;
}

static void *events_thread(void *d)
{
    struct event_t *event = NULL;
    struct timespec ts;
#ifdef __debug_events
    log("Events thread start\n");
#endif
    while (running) {
        pthread_mutex_lock(&mutex);
        if (list_empty(&g_events))
            pthread_cond_wait(&cond, &mutex);

        event = list_top(&g_events, struct event_t, children);
        list_del(&event->children);
        if (!event)
            continue;

        if (convert_time(&ts, event->delay))
            pthread_cond_timedwait(&cond, &mutex, &ts);

        pthread_mutex_unlock(&mutex);
        (*event->start_routine) (event->param);
        free(event);
    }

    /*
     * the thread was stopped by setting "running" to false...
     * but before we exit, fire any events waiting on the list.
     */
#ifdef __debug_events
    print("Executing all of the remaining events...\n");
#endif
    for (;;) {
        event = list_top(&g_events, struct event_t, children);
        if (!event)
            break;

        if (convert_time(&ts, event->delay))
            pthread_cond_timedwait(&cond, &mutex, &ts);

        (*event->start_routine) (event->param);
        list_del(&event->children);
        free(event);
    }
#ifdef __debug_events
    print("done\n");
#endif
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
#ifdef __debug_events
    log("Stopping events thread\n");
#endif
    running = false;

    pthread_join(self, NULL);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

void event_add(int32_t delay, event_start_routine start, void *p)
{
    struct event_t *event;
    bool empty = list_empty(&g_events);
    if (!start)
        return;

    xmalloc(event, sizeof(struct event_t), return);
    event->delay = delay;
    event->start_routine = start;
    event->param = p;

    pthread_mutex_lock(&mutex);
    list_add(&g_events, &event->children);

    if (empty)
        pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

