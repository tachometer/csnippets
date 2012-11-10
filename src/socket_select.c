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
#if defined __use_select

#include "socket.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

struct sock_events {
    fd_set active_fd_set, read_fd_set;
};

void *__socket_set_init(int fd)
{
    struct sock_events *ev = malloc(sizeof(struct sock_events));
    if (!ev)
        return NULL;

    FD_ZERO(&ev->active_fd_set);
    FD_SET(fd, &ev->active_fd_set);

    return ev;
}

void __socket_set_deinit(void *p)
{
    free(p);
}

void __socket_set_add(void *p, int fd)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (!evs)
        return;
    FD_SET(fd, &evs->active_fd_set);
}

void __socket_set_del(void *p, int fd)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (!evs)
        return;
    FD_CLR(fd, &evs->active_fd_set);
}

int __socket_set_poll(void *p)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (!evs)
        return 0;

    evs->read_fd_set = evs->active_fd_set;
    if (select(FD_SETSIZE, &evs->read_fd_set, NULL, NULL, NULL) < 0)
        return 0;

    return FD_SETSIZE;
}

int __socket_set_get_active_fd(void *p, int i)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (!evs)
        return -1;

    if (FD_ISSET(i, &evs->read_fd_set))
        return i;
    return -1;
}

#endif
