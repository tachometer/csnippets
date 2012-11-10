/*
 * Copyright (C) 2012  asamy <f.fallen45@gmail.com>
 *
 * The first few static net functions are borrowed from ccan.
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
#include "asprintf.h"

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

static struct addrinfo *net_client_lookup(const char *hostname,
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
    long flags;
#ifndef _WIN32
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
    saved_errno = errno;
    for (i = 0; i < num; i++)
        if (pfd[i].fd != sockfd)
            close(pfd[i].fd);
    errno = saved_errno;
    return sockfd;
}

static void poll_on_client(connection_t *conn, void *sock_events)
{
    if (!conn)
        return;

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

        if (conn && conn->on_read)
            conn->on_read(conn, buffer, count);
    }

    if (done && conn) {
        __socket_set_del(sock_events, conn->fd);
        list_del(&conn->node);
        if (conn->on_disconnect)
            conn->on_disconnect(conn);
    }
}

static void poll_on_server(socket_t *socket)
{
    int n_fds, i;
    void *sock_events;
    connection_t *conn;
    socklen_t len = sizeof(struct sockaddr_in);

    sock_events = __socket_set_init(socket->fd);
    __socket_set_add(sock_events, socket->fd);
    for (;;) {
        n_fds = __socket_set_poll(sock_events);
        for (i = 0; i < n_fds; i++) {
            int active_fd = __socket_set_get_active_fd(sock_events, i);
            if (active_fd < 0)
                continue;

            if (socket->fd == active_fd) {
                for (;;) {
                    struct sockaddr_in their_addr;
                    int their_fd;

                    their_fd = accept(socket->fd, (struct sockaddr *)&their_addr, &len);
                    if (their_fd == -1) {
                        if (ERRNO == E_AGAIN || ERRNO == E_BLOCK) {
                            /* We have processed all incoming
                             * connections */
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                        continue;
                    }

                    conn = malloc(sizeof(*conn));
                    if (!conn) {
                        close(their_fd);
                        continue;
                    }
                    conn->fd = their_fd;

                    if (!set_nonblock(conn->fd, true)) {
                        close(conn->fd);
                        free(conn);
                        continue;
                    }

                    strncpy(conn->ip, inet_ntoa(their_addr.sin_addr), 16);
                    conn->last_active = time(NULL);
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
                    __socket_set_add(sock_events, their_fd);
                }
            } else {
                list_for_each(&socket->conn->children, conn, node) {
                    if (conn->fd == active_fd)
                        break;
                }
                if (conn)
                    poll_on_client(conn, sock_events);
            }
        }
    }
}

socket_t *socket_create(void)
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
    ret->conn = NULL;
    ret->fd = -1;
    return ret;
}

void socket_connect(connection_t *conn, const char *addr, const char *service)
{
    struct addrinfo *address;
    void *sock_events;

    if (conn->fd > 0)
        close(conn->fd);

    address = net_client_lookup(addr, service, AF_INET, SOCK_STREAM);
    if (!address)
        return;

    if ((conn->fd = net_connect(address)) < 0) {
        free(address);
        return;
    }

    if (conn->on_connect)
        conn->on_connect(conn);
    free(address);

    sock_events = __socket_set_init(conn->fd);
    __socket_set_add(sock_events, conn->fd);
    while (true) {
        int fd = __socket_set_poll(sock_events);
        if (fd < 0) {
            /* dead socket? */
            break;
        }
        poll_on_client(conn, sock_events);
    }
}

void socket_listen(socket_t *sock, const char *address, int32_t port, long max_conns)
{
    struct sockaddr_in srv;
    int reuse_addr = 1;
    if (!sock)
        return;

    if (sock->fd < 0)
        sock->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock->fd < 0)
        return;

    if (!set_nonblock(sock->fd, true))
        return;

    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = !address ? INADDR_ANY : inet_addr(address);
    srv.sin_port = htons(port);

    setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    if (bind(sock->fd, (struct sockaddr *)&srv, sizeof(srv)) == -1)
        return;

    if (listen(sock->fd, max_conns) == -1)
        return;

    poll_on_server(sock);
}

int socket_write(connection_t *conn, const char *fmt, ...)
{
    char *data;
    int len;
    int err;
    va_list va;

    if (!conn || conn->fd < 0)
        return EINVAL;

    va_start(va, fmt);
    len = vasprintf(&data, fmt, va);
    va_end(va);

    if (!data)
        return ENOMEM;

    do
        err = send(conn->fd, data, len, 0);
    while (err == -1 && ERRNO == EINTR);
    if (err < 0) {
        err = ERRNO;
        goto out;
    }

    if (err != len)
        fprintf(stderr,
                "socket_write(): the data sent may be incomplete!\n");
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
        conn = list_top(&socket->conn->children, connection_t,
                        node);
        if (!conn)
            break;

        if (conn->fd)
            close(conn->fd);
        list_del(&conn->node);
        free(conn);
    }

    free(socket);
}

