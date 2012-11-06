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
#ifdef __use_select
#include "socket.h"

static fd_set active_fd_set, read_fd_set;

void __socket_set_init(int fd)
{
    FD_ZERO(&active_fd_set);
    FD_SET(fd, &active_fd_set);
}

__inline__ void __socket_set_deinit(void)
{
    /* nothing */
}

void __socket_set_add(int fd)
{
    FD_SET(fd, &active_fd_set);
}

void __socket_set_del(int fd)
{
    FD_CLR(fd, &active_fd_set);
}

int __socket_set_poll(int fd)
{
    read_fd_set=active_fd_set;
    if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        return 0;
    return FD_SETSIZE;
}

int __socket_set_get_active_fd(int i)
{
    if (FD_ISSET(i, &read_fd_set))
        return i;
    return -1;
}

#endif

