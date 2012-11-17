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

struct sk_buff {
    char *data;
    size_t size;
};

struct socket {
    int fd;

    struct list_head children;      /* The list of conn */
    connection_t *conn;             /* The head connection */
    unsigned int num_connections;   /* Current active connections */

    bool accept_connections;        /* If set to false, every incoming connection will be closed,
                                       old ones, will still be there.  */
    pthread_mutex_t conn_lock;      /* The connection lock, for adding new connections,
                                       removing dead ones, incrementing number of active connections */

    void *events;                   /* Internal usage  */
    /**
     * on_accept() this callback is called whenever a new connection
     * is accepted.  self is this socket, conn is obviously the 
     * accepted connection.
     *
     * See below for more information on what to do when this function is called.
     */
    void (*on_accept) (socket_t *self, connection_t *conn);
};

#define DEFAULT_READ_SIZE 4096   /* This size is set to the buffer on the stack,
                                    see below.  */
struct connection {
    int fd;              /* The socket file descriptor */
    char ip[16];         /* The IP of this connection */
    char *remote;        /* Who did we connect to?  Or who did we come from?  */
    time_t last_active;  /* The timestamp of last activity.  Useful for PING PONG. */
    bool auto_read;      /* If enabled, this connection will be auto read otherwise,
                            the user must call socket_read() manually.  */

    /* Called when we've have connected.  This is the root of the connection.  
     * It should be used to setup other callbacks!  */
    void (*on_connect) (connection_t *self);
    /* Called when we've disconnected.   */
    void (*on_disconnect) (connection_t *self);
    /* Called when anything is read.  */
    void (*on_read) (connection_t *self, const struct sk_buff *buff);
    /* Called when this connection writes something */
    void (*on_write) (connection_t *self, const struct sk_buff *buff);

    struct list_node node;   /* The node */
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
        const char *service, long max_conns);

/**
 * Write on a socket connection
 *
 * @param conn a connection created by socket_connect() or from the listening socket.
 * @param data the data to send
 * @return errno
 *
 * Calls conn->on_write if present.
 */
extern int socket_write(connection_t *conn, const char *fmt, ...);

/**
 * socket_remove() - close and free a socket connection.
 *
 * @param socket, the listening socket.
 * @param conn, must be a connection of `socket'.
 *
 * returns true on success, false otherwise.
 *
 * This function calls connection_free() after removal from the
 * `socket' connections.
 */
extern bool socket_remove(socket_t *socket, connection_t *conn);

/**
 * socket_read() - read from a socket
 *
 * @param conn a connected socket
 * @param sk_buff the buffer that would be allocated with the data read.
 * @param size how many characters to read?
 *
 * on successfull read, this function returns true and does the following:
 *   * calls conn->on_read if not NULL.
 *   * sets the data read to `buff'.
 * on failure, this function returns false.
 *   if that conn is part of a listening socket (aka a child connection of
 *   socket_t), consider calling socket_remove() with the listening socket
 *   as a param.
 *
 * This function heap allocates memory for the data read, this means
 * buff->data must be free'd later on to avoid memory leak issues.
 */
extern bool socket_read(connection_t *conn, struct sk_buff *buff, size_t size);

#endif    /* __socket_h */

