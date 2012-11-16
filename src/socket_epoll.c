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
#ifdef __use_epoll

#define MAX_EVENTS 1024
#include <sys/epoll.h>
#include <unistd.h>   /* close() */

struct sock_events {
    struct epoll_event *events;
    int epoll_fd;
};

void *__socket_set_init(int fd)
{
    struct sock_events *ev = malloc(sizeof(struct sock_events));
    if (!ev)
        return NULL;

    ev->epoll_fd = epoll_create1(0);
    if (ev->epoll_fd == -1) {
        perror("epoll_create1");
        abort();
    }

    ev->events = calloc(MAX_EVENTS, sizeof(struct epoll_event));
    if (!ev->events) {
        perror("calloc");
        abort();
    }

    return ev;
}

void __socket_set_deinit(void *p)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (unlikely(!evs))
        return;
    free(evs->events);
    free(evs);
}

void __socket_set_add(void *p, int fd)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (unlikely(!evs))
        return;

    struct epoll_event ev = {
        .data.fd = fd,
        .events = EPOLLIN | EPOLLET
    };

    if (epoll_ctl(evs->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
        perror("epoll_ctl failed");
}

void __socket_set_del(int fd)
{
    /* nothing... */
}

int __socket_set_poll(void *p)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (unlikely(!evs))
        return 0;
    return epoll_wait(evs->epoll_fd, evs->events, MAX_EVENTS, -1);
}

int __socket_set_get_active_fd(void *p, int fd)
{
    uint32_t events;
    struct sock_events *evs = (struct sock_events *)p;
    if (unlikely(!evs))
        return -1;

    events = evs->events[fd].events;
    if (events & EPOLLERR || events & EPOLLHUP || !(events & EPOLLIN)) {
        close(evs->events[fd].data.fd);
        return -1;
    }
    return evs->events[fd].data.fd;
}

#endif

