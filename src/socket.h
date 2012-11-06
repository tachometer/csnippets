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
#include <time.h>
#include <stdint.h>

/**
 * The following enum determines what type of socket has been created
 * STREAM_CLIENT if it's a client connection
 * STREAM_SERVER if it's a listening socket.
 */
typedef enum {
    STREAM_CLIENT = 0x00,
    STREAM_SERVER = 0x01
} socket_type_t;

typedef struct socket socket_t;
typedef struct connection connection_t;

struct socket {
    socket_type_t type;
    int fd;
    connection_t *conn;

    void (*on_accept) (socket_t *self, connection_t *conn);
};

struct connection {
    int fd;
    char ip[16];
    time_t last_active;

    void (*on_connect) (connection_t *self);
    void (*on_disconnect) (connection_t *self);
    void (*on_read) (connection_t *self, const char *read, int len);
    void (*on_write) (connection_t *self, const char *written, int len);

    struct list_node node;
    struct list_head children;
};

/**
 * Create socket with a NULL connection.
 * 
 * To create the initial connection, call socket_connect() or
 * socket_listen() on the socket.
 * 
 * @return a malloc'd socket or NULL on failure.
 */
extern socket_t *socket_create(void);

/**
 * Free socket and all of it's connections if any.
 * 
 * @param socket a socket returned by socket_create().
 * @return nothing.
 */
extern void socket_free(socket_t *socket);

/**
 * socket_connect() - connect to a server.
 * @param conn a malloc'd connection that has atleast the on_connect callback setup.
 * @param socket the socket that was created by socket_create()
 * @param addr the address the server is listening on.
 * @param service port or a register service.
 *
 * @return true on success, false otherwise.
 */
extern bool socket_connect(connection_t *conn, socket_t *socket, const char *addr, const char *service);

/**
 * Listen on socket.
 * 
 * @param address can be NULL if indepdent.
 * @param port port to listen on.
 *
 * @return true on success, false otherwise.
 */
extern bool socket_listen(socket_t *socket, const char *address, int32_t port, long max_conns);

/**
 * Poll on \a socket
 * 
 * @param socket a socket returned by socket_create() and have had
 *      socket_listen() or socket_connect() called on.
 * 
 * @return never.
 */
extern void socket_poll(socket_t *socket);

#endif    /* __socket_h */
