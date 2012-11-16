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

#include <pthread.h>
#include <pthread.h>
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

        if (unlikely(!event))
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

    running = true;
    rc = pthread_create(&self, &attr, &events_thread, NULL);
    if (rc != 0)
        fatal("failed to create thread");
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
    if (unlikely(!start || delay < 0))
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
    if (likely(running)) {
        empty = list_empty(&events);
        list_add(&events, &event->node);
    } else
        printf("attempting to add an event to a terminated event queue\n");

    pthread_mutex_unlock(&mutex);
    if (empty)
        pthread_cond_signal(&cond);
}

