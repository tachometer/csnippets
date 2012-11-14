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

