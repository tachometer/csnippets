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
 *
 * TODO: UDP support.
 */
#ifndef __socket_h
#define __socket_h

#include "list.h"

__begin_header

#define STREAM_SERVER 0x1

#define MAX_EVENTS 1024
#define sock_cast(self) (struct socket_t *)(self)
struct socket_t {
    int32_t fd;
    char ip[16];
    time_t idle;
    int32_t type;

    struct {
        void (*on_read) (void *self, const char *read, int len);
        void (*on_accept) (void *self, void *new_socket);
        void (*on_write) (void *self, const char *written, int len);
        void (*on_disconnect) (void *self);
        void (*on_connect) (void *self);
    };
    struct {
        int (*write) (void *self, const char *data, int len);
        int (*close) (void *self);
        int (*shutdown) (void *self);
    };

    struct list_node node;
    struct list_head children;
};

extern struct socket_t *__warn_unused socket_create(void);
extern void socket_close(struct socket_t *socket);
extern bool socket_connect(struct socket_t *socket, const char *addr,
        int32_t port);
extern bool socket_listen(struct socket_t *socket, const char *address,
        int32_t port);
extern void socket_poll(struct socket_t *socket);

__end_header
#endif    /* __socket_h */

