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
 * TODO:
 *  * UDP support.
 *  * SSL support.
 */
#ifndef __socket_h
#define __socket_h

#include "list.h"

#include <time.h>
#include <pthread.h>

typedef struct socket socket_t;
typedef struct connection connection_t;

struct socket {
    int fd;

    struct list_head children;
    connection_t *conn;
    unsigned int num_connections;
    bool accept_connections;
    pthread_mutex_t conn_lock;

    void (*on_accept) (socket_t *self, connection_t *conn);
};

struct connection {
    int fd;
    char ip[16];
    char *remote;
    time_t last_active;

    void (*on_connect) (connection_t *self);
    void (*on_disconnect) (connection_t *self);
    void (*on_read) (connection_t *self, const char *read, int len);
    void (*on_write) (connection_t *self, const char *written, int len);

    struct list_node node;
};

/**
 * Create socket with a NULL connection.
 *
 * @return a malloc'd socket or NULL on failure.
 */
extern socket_t *socket_create(void (*on_accept) (socket_t *, connection_t *));

/**
 * Free socket and all of it's connections if any.
 *
 * @param socket a socket returned by socket_create().
 */
extern void socket_free(socket_t *socket);

/**
 * Create a connection
 *
 * @param on_connect, the callback to use for on_connect
 */
connection_t *connection_create(void (*on_connect) (connection_t *));

/**
 * Close & Free the connection
 *
 * @param conn a socket created by connection_create()
 */
extern void connection_free(connection_t *conn);

/**
 * socket_connect() - connect to a server.
 *
 * @param conn a malloc'd connection that has atleast the on_connect callback setup.
 * @param addr the address the server is listening on.
 * @param service port or a register service.
 */
extern bool socket_connect(connection_t *conn, const char *addr,
        const char *service);

/**
 * Listen on socket.
 *
 * @param address can be NULL if indepdent.
 * @param port port to listen on.
 */
extern bool socket_listen(socket_t *socket, const char *address,
        int32_t port, long max_conns);

/**
 * Write on a socket connection
 *
 * @param conn a connection created by socket_connect() or from the listening socket.
 * @param data the data to send
 * @return errno
 */
extern int socket_write(connection_t *conn, const char *fmt, ...);

#endif    /* __socket_h */

