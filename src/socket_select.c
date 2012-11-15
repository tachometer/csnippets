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
    return ev;
}

void __socket_set_deinit(void *p)
{
    free(p);
}

void __socket_set_add(void *p, int fd)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (unlikely(!evs))
        return;
    FD_SET(fd, &evs->active_fd_set);
}

void __socket_set_del(void *p, int fd)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (unlikely(!evs))
        return;
    FD_CLR(fd, &evs->active_fd_set);
}

int __socket_set_poll(void *p)
{
    int ret;
    struct sock_events *evs = (struct sock_events *)p;
    if (unlikely(!evs))
        return 0;

    evs->read_fd_set = evs->active_fd_set;
    ret = select(FD_SETSIZE, &evs->read_fd_set, NULL, NULL, NULL);
    if (ret < 0)
        return 0;

    return ret;
}

int __socket_set_get_active_fd(void *p, int fd)
{
    struct sock_events *evs = (struct sock_events *)p;
    if (unlikely(!evs))
        return -1;

    if (FD_ISSET(fd, &evs->read_fd_set))
        return fd;
    return -1;
}

#endif

