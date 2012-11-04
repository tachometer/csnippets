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
#include "socket.h"
#include <sys/epoll.h>

static struct epoll_event *m_events;
static int epoll_fd;

void __socket_set_init(const struct socket_t *socket)
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
        log_errno("failed to create epoll fd");

    xcalloc(m_events, MAX_EVENTS, sizeof(struct epoll_event),
            fatal("callocing events!\n"));
}

void __socket_set_deinit(void)
{
    free(m_events);
}

void __socket_set_add(const struct socket_t *socket)
{
    struct epoll_event ev = {
        .data.fd = socket->fd,
        .events = EPOLLIN | EPOLLET
    };

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket->fd, &ev) == -1)
        log_errno("epoll_ctl failed");
}

void __socket_set_del(const struct socket_t *socket)
{
    /* nothing... */
}

int __socket_set_poll(const struct socket_t *socket)
{
    return epoll_wait(epoll_fd, m_events, MAX_EVENTS, -1);  
}

int __socket_set_get_active_fd(int i)
{
    if (m_events[i].events & EPOLLERR ||
            m_events[i].events & EPOLLHUP ||
            !(m_events[i].events & EPOLLIN)) {
        close(m_events[i].data.fd);
        return -1;
    }
    return m_events[i].data.fd;
}

#endif

