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

