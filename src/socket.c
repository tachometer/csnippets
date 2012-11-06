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
#include "socket.h"
#include "platform.h"
#include "net.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>    /* socklen_t */
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#endif
#include <fcntl.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern void __socket_set_init(int fd);
extern void __socket_set_deinit(void);
extern void __socket_set_add(int fd);
extern void __socket_set_del(int fd);
extern int  __socket_set_poll(int fd);
extern int  __socket_set_get_active_fd(int i);

#ifdef _WIN32
static bool is_initialized = false;
#endif

socket_t *socket_create(void)
{
    socket_t *ret = malloc(sizeof(*ret));
    if (!ret)
        return NULL;
    ret->conn = NULL;
    ret->fd = -1;
    ret->type = -1;
    return ret;
}

bool socket_connect(connection_t *conn,
                    socket_t *socket, const char *addr, const char *service)
{
    struct addrinfo *address;

    if (!socket || !conn)
        return NULL;

    if (socket->fd > 0)
        close(socket->fd);

    address = net_client_lookup(addr, service, AF_INET, SOCK_STREAM);
    if (!addr)
        return NULL;

    if ((conn->fd = net_connect(address)) < 0) {
        free(address);
        return NULL;
    }

    socket->type  = STREAM_CLIENT;
    if (conn->on_connect)
        conn->on_connect(conn);
    free(address);

    socket->conn = conn;
    socket->fd = conn->fd;
    socket->type = STREAM_CLIENT;
    return conn;
}

bool socket_listen(socket_t *sock, const char *address, int32_t port, long max_conns)
{
    struct sockaddr_in srv;
    int reuse_addr = 1;
    if (!sock)
        return false;

    if (sock->fd < 0) {
        sock->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock->fd < 0)
            return false;
    }

    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = !address ? INADDR_ANY : inet_addr(address);
    srv.sin_port = htons(port);

    setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    if (bind(sock->fd, (struct sockaddr *)&srv, sizeof(srv)) == -1)
        return false;

    if (listen(sock->fd, max_conns) == -1)
        return false;

    sock->type = STREAM_SERVER;
    return true;
}

void socket_poll(socket_t *socket)
{
    int n_fds, i;
    connection_t *conn;
    socklen_t len = sizeof(struct sockaddr_in);

    __socket_set_init(socket->fd);
    __socket_set_add(socket->fd);
    for (;;) {
        n_fds = __socket_set_poll(socket->fd);
        for (i = 0; i < n_fds; i++) {
            int active_fd = __socket_set_get_active_fd(i);
            if (active_fd < 0)
                continue;

            if (socket->fd == active_fd) {
                if (socket->type == STREAM_SERVER) {
                    struct sockaddr_in their_addr;
                    int their_fd;

                    their_fd = accept(socket->fd, (struct sockaddr *)&their_addr, &len);
                    if (their_fd == -1) {
                        if (ERRNO != E_AGAIN && ERRNO != E_BLOCK)
                            perror("accept()");
                        continue;
                    }

                    conn = malloc(sizeof(*conn));
                    if (!conn)
                        goto out;

                    conn->fd = their_fd;
                    conn->last_active = time(NULL);

                    if (!set_nonblock(conn->fd, true))
                        goto out;

                    strncpy(conn->ip, inet_ntoa(their_addr.sin_addr), 16);

                    if (socket->on_accept) {
                        socket->on_accept(socket, conn);
                        if (conn->on_connect)
                            conn->on_connect(conn);
                    }

                    if (!socket->conn) {
                        socket->conn = conn;
                        list_head_init(&conn->children);
                    }
                    list_add(&socket->conn->children, &conn->node);
                    __socket_set_add(conn->fd);
                    continue;
                } else {
                    /*
                     * if this is not a server socket, mark the `conn' to be
                     * the connection of this socket so the next code can use it safely
                     */
                    conn = socket->conn;
                }
            }

            /**
             * If this is a server, find the first child connection
             * that matches the active fd.
             */
            if (socket->type == STREAM_SERVER && !list_empty(&socket->conn->children)) {
                list_for_each(&socket->conn->children, conn, node) {
                    if (conn->fd == active_fd)
                        break;   /* got it. */
                }
            }

            /**
             * This is a client socket, read everything waiting
             * if there's nothing/error, close the connection.
             */
            bool done = false;
            for (;;) {
                ssize_t count;
                char buffer[4096];

                count = read(active_fd, buffer, sizeof buffer);
                if (count == -1) {
                    if (ERRNO != E_AGAIN && errno != E_BLOCK)
                        done = true;
                    break;
                } else if (count == 0) { /* EOF */
                    done = true;
                    break;
                }

                if (conn && conn->on_read)
                    conn->on_read(conn, buffer, count);
            }

            /**
             * An error occured during read()
             * close the connection
             */
            if (done && conn) {
                list_del(&conn->node);
                if (conn->on_disconnect)
                    conn->on_disconnect(conn);
            }
        }
    }

out:
    close(conn->fd);
    free(conn);
}

void socket_free(socket_t *socket)
{
    connection_t *conn;
    if (!socket)
       return;

    for (;;) {
        conn = list_top(&socket->conn->children, connection_t,
                        node);
        if (!conn) {
            /* We're done.  */
            break;
        }

        if (conn->fd)
            close(conn->fd);
        list_del(&conn->node);
        free(conn);
    }

    free(socket);
}

