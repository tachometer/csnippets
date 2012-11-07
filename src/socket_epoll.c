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
#ifdef __use_epoll

#define MAX_EVENTS 1024
#include <sys/epoll.h>

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
    if (!evs)
        return;
    free(evs->events);
    free(evs);
}

void __socket_set_add(void *p, int fd)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (!evs)
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
    if (!evs)
        return;
    return epoll_wait(evs->epoll_fd, evs->events, MAX_EVENTS, -1);
}

int __socket_set_get_active_fd(void *p, int i)
{
    uint32_t events;
    struct sock_events *evs = (struct sock_events *)p;
    if (!evs)
        return -1;

    events = evs->events[i].events;
    if (events & EPOLLERR || events & EPOLLHUP || !(events & EPOLLIN)) {
        close(evs->events[i].data.fd);
        return -1;
    }
    return evs->events[i].data.fd;
}

#endif

