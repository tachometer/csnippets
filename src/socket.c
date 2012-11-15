/*
 * Copyright (c) 2012 asamy <f.fallen45@gmail.com>
 *
 * The first few static net_* functions are borrowed from ccan/net.
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
#include "socket.h"
#include "asprintf.h"
#include "atomic.h"

#ifdef _WIN32
#define poll WSAPoll
#define pollfd WSAPOLLFD
#include <winsock2.h>
#include <ws2tcpip.h>    /* socklen_t */
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
typedef struct pollfd pollfd;
#endif
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern void *__socket_set_init(int);
extern void __socket_set_deinit(void *);
extern void __socket_set_add(void *, int);
extern void __socket_set_del(void *, int);
extern int  __socket_set_poll(void *);
extern int  __socket_set_get_active_fd(void *, int);

#ifdef _WIN32
static bool is_initialized = false;
#endif

static void add_connection(socket_t *socket, connection_t *conn)
{
    pthread_mutex_lock(&socket->conn_lock);

    list_add(&socket->children, &conn->node);
    atomic_ref(&socket->num_connections);

    pthread_mutex_unlock(&socket->conn_lock);
}

static void rm_connection(socket_t *socket, connection_t *conn)
{
    pthread_mutex_lock(&socket->conn_lock);

    list_del_from(&socket->children, &conn->node);
    atomic_deref(&socket->num_connections);

    pthread_mutex_unlock(&socket->conn_lock);
}

static struct addrinfo *net_lookup(const char *hostname,
        const char *service,
        int family,
        int socktype)
{
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if (getaddrinfo(hostname, service, &hints, &res) != 0)
        return NULL;

    return res;
}

static bool set_nonblock(int fd, bool nonblock)
{
#ifndef _WIN32
    long flags;
    flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        return false;

    if (nonblock)
        flags |= O_NONBLOCK;
    else
        flags &= ~(long)O_NONBLOCK;

    return (fcntl(fd, F_SETFL, flags) == 0);
#else
    return ioctlsocket(fd, FIONBIO, (long *)&nonblock) == 0;
#endif
}

/* We only handle IPv4 and IPv6 */
#define MAX_PROTOS 2

static void remove_fd(pollfd *pfd, const struct addrinfo **addr,
        socklen_t *slen, unsigned int *num,
        unsigned int i)
{
    memmove(pfd + i, pfd + i + 1, (*num - i - 1) * sizeof(pfd[0]));
    memmove(addr + i, addr + i + 1, (*num - i - 1) * sizeof(addr[0]));
    memmove(slen + i, slen + i + 1, (*num - i - 1) * sizeof(slen[0]));
    (*num)--;
}

static int net_connect(const struct addrinfo *addrinfo)
{
    int sockfd = -1, saved_errno;
    unsigned int i, num;
    const struct addrinfo *ipv4 = NULL, *ipv6 = NULL;
    const struct addrinfo *addr[MAX_PROTOS];
    socklen_t slen[MAX_PROTOS];
    pollfd pfd[MAX_PROTOS];

    for (; addrinfo; addrinfo = addrinfo->ai_next) {
	switch (addrinfo->ai_family) {
	case AF_INET:
	    if (!ipv4)
		ipv4 = addrinfo;
	    break;
	case AF_INET6:
	    if (!ipv6)
		ipv6 = addrinfo;
	    break;
	}
    }

    num = 0;
    /* We give IPv6 a slight edge by connecting it first. */
    if (ipv6) {
	addr[num] = ipv6;
	slen[num] = sizeof(struct sockaddr_in6);
	pfd[num].fd = socket(AF_INET6, ipv6->ai_socktype,
				ipv6->ai_protocol);
	if (pfd[num].fd != -1)
	    num++;
    }

    if (ipv4) {
	addr[num] = ipv4;
	slen[num] = sizeof(struct sockaddr_in);
	pfd[num].fd = socket(AF_INET, ipv4->ai_socktype,
				ipv4->ai_protocol);
	if (pfd[num].fd != -1)
	    num++;
    }

    for (i = 0; i < num; i++) {
        if (!set_nonblock(pfd[i].fd, true)) {
            remove_fd(pfd, addr, slen, &num, i--);
            continue;
        }
        /* Connect *can* be instant. */
        if (connect(pfd[i].fd, addr[i]->ai_addr, slen[i]) == 0)
            goto got_one;
        if (ERRNO != E_INPROGRESS) {
            /* Remove dead one. */
            remove_fd(pfd, addr, slen, &num, i--);
        }
        pfd[i].events = POLLOUT;
    }

    while (num && poll(pfd, num, -1) != -1) {
        for (i = 0; i < num; i++) {
	    int err;
            socklen_t errlen = sizeof(err);
            if (!pfd[i].revents)
                continue;
            if (getsockopt(pfd[i].fd, SOL_SOCKET, SO_ERROR, &err, &errlen) != 0)
                goto out;
            if (err == 0)
                goto got_one;

            /* Remove dead one. */
            errno = err;
            remove_fd(pfd, addr, slen, &num, i--);
        }
    }

got_one:
    /* We don't want to hand them a non-blocking socket! */
    if (set_nonblock(pfd[i].fd, false))
        sockfd = pfd[i].fd;

out:
    saved_errno = ERRNO;
    for (i = 0; i < num; i++)
        if (pfd[i].fd != sockfd)
            close(pfd[i].fd);
    set_last_error(saved_errno);
    return sockfd;
}

static bool __poll_on_client(socket_t *parent_socket, connection_t *conn, void *sock_events)
{
    if (!conn)
        return false;

    bool done = false;
    for (;;) {
        ssize_t count;
        char buffer[4096];

        count = read(conn->fd, buffer, sizeof buffer);
        if (count == -1) {
            if (ERRNO != E_AGAIN && errno != E_BLOCK)
                done = true;
            break;
        } else if (count == 0) { /* EOF */
            done = true;
            break;
        }

        buffer[count] = '\0';
        if (conn && conn->on_read)
            conn->on_read(conn, buffer, count);
    }

    if (done && conn) {
        __socket_set_del(sock_events, conn->fd);

        if (parent_socket)
            rm_connection(parent_socket, conn);

        connection_free(conn);
        return false;
    }

    return true;
}

static void *poll_on_client(void *client)
{
    connection_t *conn = (connection_t *)client;
    void *sock_events;

    sock_events = __socket_set_init(conn->fd);
    if (!sock_events)
        return NULL;

    __socket_set_add(sock_events, conn->fd);
    for (;;) {
        int fd = __socket_set_poll(sock_events);
        if (fd < 0) {
            __socket_set_deinit(sock_events);
            break;
        }

        if (!__poll_on_client(NULL, conn, sock_events))
            break;
    }

    return NULL;
}

static void *poll_on_server(void *_socket)
{
    socket_t *socket = (socket_t *)_socket;
    int n_fds, fd;
    void *sock_events;
    connection_t *conn;
    socklen_t len = sizeof(struct sockaddr_in);

    sock_events = __socket_set_init(socket->fd);
    if (!sock_events)
        return NULL;

    __socket_set_add(sock_events, socket->fd);
    for (;;) {
        n_fds = __socket_set_poll(sock_events);
        for (fd = 0; fd < n_fds; ++fd) {
            int active_fd = __socket_set_get_active_fd(sock_events, fd);
            if (active_fd < 0)
                continue;

            if (socket->fd == active_fd) {
                /* Loop until we have finished every single connection waiting */
                for (;;) {
                    struct sockaddr_in their_addr;
                    int their_fd;

                    their_fd = accept(socket->fd, (struct sockaddr *)&their_addr, &len);
                    if (their_fd == -1) {
                        if (ERRNO == E_AGAIN || ERRNO == E_BLOCK) {
                            /* We have processed all of the incoming connections */
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                        continue;
                    }

                    if (!socket->accept_connections) {
                        __socket_set_del(sock_events, their_fd);
                        close(their_fd);
                        continue;
                    }

                    xmalloc(conn, sizeof(*conn), close(their_fd); continue);
                    conn->fd = their_fd;

                    if (!set_nonblock(conn->fd, true)) {
                        close(conn->fd);
                        free(conn);
                        continue;
                    }

                    strncpy(conn->ip, inet_ntoa(their_addr.sin_addr), 16);
                    conn->last_active = time(NULL);
                    /*  TODO: set conn->remote here */

                    if (likely(socket->on_accept)) {
                        socket->on_accept(socket, conn);
                        if (likely(conn->on_connect))
                            conn->on_connect(conn);
                    }

                    if (unlikely(!socket->conn))
                        socket->conn = conn;

                    add_connection(socket, conn);
                    __socket_set_add(sock_events, their_fd);
                }
            } else {
                list_for_each(&socket->children, conn, node) {
                    if (conn->fd == active_fd)
                        break;
                }
                if (conn)
                    __poll_on_client(socket, conn, sock_events);
            }
        }
    }

    return NULL;
}

socket_t *socket_create(void (*on_accept) (socket_t *, connection_t *))
{
    socket_t *ret = malloc(sizeof(*ret));
#ifdef _WIN32
    WSADATA wsa;
    if (!is_initialized) {
        if (!WSAStartup(MAKEWORD(2,2), &wsa)) {
            WSACleanup();
            fprintf(stderr, "WSAStartup() failed\n");
            abort();
        }
        initialized = true;
    }
#endif
    if (!ret)
        return NULL;

    ret->on_accept = on_accept;
    ret->conn = NULL;
    ret->fd = -1;
    return ret;
}

connection_t *connection_create(void (*on_connect) (connection_t *))
{
    connection_t *ret;

    ret = malloc(sizeof(*ret));
    if (!ret)
        return NULL;

    ret->on_connect = on_connect;
    ret->last_active = 0;
    ret->fd = -1;
    ret->remote = NULL;
    return ret;
}

bool socket_connect(connection_t *conn, const char *addr, const char *service)
{
    struct addrinfo *address;
    pthread_t thread;
    pthread_attr_t attr;
    int ret;

    if (conn->fd > 0)
        close(conn->fd);

    address = net_lookup(addr, service, AF_INET, SOCK_STREAM);
    if (!address)
        return false;

    if ((conn->fd = net_connect(address)) < 0) {
        freeaddrinfo(address);
        return false;
    }
    freeaddrinfo(address);

    conn->remote = addr;
    if (likely(conn->on_connect))
        conn->on_connect(conn);

    pthread_attr_init(&attr);
    if ((ret = pthread_create(&thread, &attr, poll_on_client,
                    (void *)conn)) != 0) {
        fprintf(stderr, "failed to create thread (%d): %s\n",
                    ret, strerror(ret));
        return false;
    }

    return true;
}

bool socket_listen(socket_t *sock, const char *address, const char *service, long max_conns)
{
    int reuse_addr = 1;
    pthread_t thread;
    pthread_attr_t attr;
    int ret;
    struct addrinfo *addr;

    if (!sock)
        return false;

    addr = net_lookup(address, service, AF_INET, SOCK_STREAM);
    if (!addr)
        return false;

    if (sock->fd < 0)
        sock->fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock->fd < 0)
        return false;

    if (!set_nonblock(sock->fd, true))
        goto out;

    setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    if (bind(sock->fd, addr->ai_addr, addr->ai_addrlen) == -1)
        goto out;

    if (listen(sock->fd, max_conns) == -1)
        goto out;

    sock->accept_connections = true;
    list_head_init(&sock->children);

    pthread_attr_init(&attr);
    if ((ret = pthread_create(&thread, &attr, poll_on_server,
                    (void *)sock)) != 0) {
        fprintf(stderr, "failed to create thread (%d): %s\n",
                     ret, strerror(ret));
        return false;
    }

    return true;
out:
    if (addr)
        freeaddrinfo(addr);
    if (sock->fd)
        close(sock->fd);
    return false;
}

int socket_write(connection_t *conn, const char *fmt, ...)
{
    char *data;
    int len;
    int err;
    va_list va;

    if (unlikely(!conn || conn->fd < 0))
        return -EINVAL;

    va_start(va, fmt);
    len = vasprintf(&data, fmt, va);
    va_end(va);

    if (!data)
        return -ENOMEM;

    do
        err = send(conn->fd, data, len, 0);
    while (err == -1 && errno == EINTR);

    if (unlikely(err < 0)) {
        err = -errno;
        goto out;
    }

    if (unlikely(err != len))
        fprintf(stderr,
                "socket_write(): the data sent may be incomplete!\n");

    if (conn->on_write)
        conn->on_write(conn, data, len);

out:
    free(data);
    return err;
}

void socket_free(socket_t *socket)
{
    connection_t *conn;
    if (!socket)
       return;

    for (;;) {
        conn = list_top(&socket->children, connection_t,
                        node);
        if (!conn)
            break;

        if (conn->fd)
            close(conn->fd);
        list_del_from(&socket->children, &conn->node);
        free(conn);
    }

    free(socket);
}

void connection_free(connection_t *conn)
{
    if (unlikely(!conn || conn->fd < 0))
        return;

    if (likely(conn->on_disconnect))
        conn->on_disconnect(conn);

    close(conn->fd);
    free(conn);
}

