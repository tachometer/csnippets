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
#include "event.h"
#include "rwlock.h"

#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

static pthread_mutex_t mutex        = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond         = PTHREAD_COND_INITIALIZER;
/**
 * TODO: This looks like a bad way of doing it...
 * wipe this out and figure out a better way of doing a
 * quit-like mutex.
 */
static rwlock_t        lock;
static bool            running;
static LIST_HEAD(g_events);

static void *events_thread(void *d)
{
    struct event_t *event = NULL;
    struct timespec ts;
    struct timeval  tv;

#ifdef __debug_events
    log("Events thread start\n");
#endif
    while (running) {
        if (rwlock_rdtrylock(&lock) == EBUSY)
            break;

        pthread_mutex_lock(&mutex);
        if (list_empty(&g_events))
            pthread_cond_wait(&cond, &mutex);

        event = list_top(&g_events, struct event_t, children);
        list_del(&event->children);
        if (!event)
            continue;

        if (event->delay > 0) {
            gettimeofday(&tv, NULL);
            ts.tv_sec  = tv.tv_sec;
            ts.tv_nsec = tv.tv_usec * 1000;
            ts.tv_sec += event->delay;

            pthread_cond_timedwait(&cond, &mutex, &ts);
        }

        pthread_mutex_unlock(&mutex);
        (*event->start_routine) (event->param);
        rwlock_rdunlock(&lock);
    }

    /*
     * the thread was stopped by setting "running" to false...
     * but before we exit, run any events waiting on the list.
     */
#ifdef __debug_events
    log("Executing all of the remaining events... ");
#endif
    list_for_each(&g_events, event, children) {
        (*event->start_routine) (event->param);
        list_del(&event->children);
    }
#ifdef __debug_events
    printf("done\n");
#endif
}

void events_init(void)
{
    pthread_t thread_id;
    pthread_attr_t thread_attr;
    int rc;

    rc = pthread_attr_init(&thread_attr);
    if (rc != 0)
        fatal("failed to initialize thread attributes\n");

    rc = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    if (rc != 0)
        fatal("failed to setdetachstate");

    rc = pthread_create(&thread_id, &thread_attr, &events_thread, NULL);
    if (rc != 0)
        fatal("failed to create thread");
    rwlock_wrlock(&lock);
    running = true;
    rwlock_wrunlock(&lock);
}

void events_stop(void)
{
    rwlock_wrlock(&lock);
    running = false;
    rwlock_wrunlock(&lock);

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

