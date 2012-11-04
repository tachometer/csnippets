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

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>    /* socklen_t */
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include <fcntl.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

extern void __socket_set_init(const struct socket_t *socket) __weak;
extern void __socket_set_deinit(void) __weak;
extern void __socket_set_add(const struct socket_t *socket) __weak;
extern void __socket_set_del(const struct socket_t *socket) __weak;
extern int  __socket_set_poll(const struct socket_t *socket) __weak;
extern int  __socket_set_get_active_fd(int i) __weak;

#ifdef _WIN32
static bool is_initialized = false;
#endif

static void setup_async(const struct socket_t *socket)
{
    int flags;
#ifndef _WIN32
    flags = fcntl(socket->fd, F_GETFL, 0);
    if (!(flags & O_NONBLOCK))
        flags |= O_NONBLOCK;

    fcntl(socket->fd, F_SETFL, flags);
#else
    flags = 1;
    ioctlsocket(socket->fd, FIONBIO, (u_long *)&flags);
#endif
}

static int __write(void *self, const char *data, int len)
{
    int32_t ret;
    struct socket_t *socket = sock_cast(self);
    if (!socket)
        return -1;

    do
        ret = send(socket->fd, data, len, 0);
    while (ret == -1 && errno == EINTR);

    if (socket->on_write)
        socket->on_write(self, data, ret);
    return ret;
}

static int __close(void *self)
{
    struct socket_t *socket = sock_cast(self);
    if (!socket)
        return -1;

    if (socket->fd > 0)
        close(socket->fd);

    __socket_set_del(socket);
    if (socket->on_disconnect)
        socket->on_disconnect(socket);
    return 0;
}

static __inline__ void setup_socket(struct socket_t *socket)
{
#ifdef _WIN32
    if (!is_initialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2,2), &wsaData);
        is_initialized=true;
    }
#endif
    socket->write = __write;
    socket->close = __close;
    socket->idle  = time(NULL);
    socket->type  = 0;
    list_head_init(&socket->children);
}

struct socket_t *socket_create(void)
{
    struct socket_t *ret;
    int32_t sockfd;

    xmalloc(ret, sizeof(struct socket_t), return NULL);
    /*
     * UDP:
     *      AF_INET -> PF_INET
     *      SOCK_STREAM -> SOCK_DGRAM
     *      IPPROTO_TCP -> IPPROTO_UDP
     */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        goto out;

    setup_async(ret);
    ret->fd = sockfd;
    setup_socket(ret);

    return ret;
out:
    xfree(ret);
    return NULL;
}

bool socket_connect(struct socket_t *socket, const char *addr, int32_t port)
{
    time_t start;
    struct hostent *hp;
    bool connected = false;
    struct sockaddr_in srv;

    if (socket->fd < -1)
        return false;
    if (!(hp = gethostbyname(addr)))
        return false;
#ifdef WIN32
    memcpy((char*)&srv.sin_addr, (char*)hp->h_addr, hp->h_length);
#else
    bcopy((char*)hp->h_addr, (char*)&srv.sin_addr, hp->h_length);
#endif

    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);

    start = time(NULL);
    while (!connected && time(NULL) - start < 10) {
        set_last_error(0);
#ifdef __debug_socket
        print("Trying %s...\n", addr);
#endif
        if (connect(socket->fd, (struct sockaddr *)&srv, sizeof(srv)) == -1) {
            switch (ERRNO) {
                case E_ISCONN:
                case E_ALREADY:
                case E_INPROGRESS:
                    connected = true;
                    break;
            }
        } else {
            connected = true;
            break;
        }
    }

#ifdef __debug_socket
    print("Successfully connected to %s\n", addr);
#endif
    if (socket->on_connect)
        socket->on_connect(socket);
    return true;
}

bool socket_listen(struct socket_t *socket, const char *address, int32_t port)
{
    struct sockaddr_in srv;
    int reuse_addr = 1;
    if (!socket)
        return false;
    if (socket->fd < 0)
        return false;

    srv.sin_family = AF_INET;          /* UDP -> PF_INET */
    srv.sin_addr.s_addr = !address ? INADDR_ANY : inet_addr(address);
    srv.sin_port = htons(port);

    setsockopt(socket->fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    if (bind(socket->fd, (struct sockaddr *)&srv, sizeof(srv)) == -1)
        return false;
    if (listen(socket->fd, MAX_EVENTS) == -1)
        return false; 
    socket->type = STREAM_SERVER;
    return true;
}

void socket_poll(struct socket_t *socket)
{
    int n_fds, i;
    __socket_set_init(socket);
    __socket_set_add(socket);
    for (;;) {
        n_fds = __socket_set_poll(socket);
        for (i = 0; i < n_fds; i++) {
            struct socket_t *sock = NULL;
            int active_fd = __socket_set_get_active_fd(i);
            if (active_fd < 0)
                continue;

            if (socket->fd == active_fd) {
                if (socket->type == STREAM_SERVER) {
                    struct socket_t *new_socket;
                    struct sockaddr_in their_addr;
                    socklen_t len = sizeof(their_addr);
                    int their_fd;

                    their_fd = accept(socket->fd, &their_addr, &len);
                    if (their_fd == -1) {
                        if (ERRNO != E_AGAIN && ERRNO != E_BLOCK)
                            log_errno("accept() on fd %d", socket->fd);
                        break;
                    }

                    xmalloc(new_socket, sizeof(*new_socket), break);
                    setup_socket(new_socket);
                    setup_async(new_socket);

                    new_socket->fd = their_fd;
                    strncpy(new_socket->ip, inet_ntoa(their_addr.sin_addr), 16);

                    if (socket->on_accept) {
                        socket->on_accept(socket, new_socket);
                        if (new_socket->on_connect)
                            new_socket->on_connect(new_socket);
                    }
                    list_add(&socket->children, &new_socket->node);
                    __socket_set_add(new_socket);
                    continue;
                }
                sock=socket;
            }

            if (socket->type == STREAM_SERVER && !list_empty(&socket->children)) {
                list_for_each(&socket->children, sock, node) {
                    if (sock->fd == active_fd)
                        break;
                }
            }

            bool done = false;
            for (;;) {
                ssize_t count;
                char buffer[4096];

                count = read(active_fd, buffer, sizeof buffer);
                if (count == -1) {
#ifndef _WIN32
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
#else
                    if (WSAGetLastError() == WSAEWOULDBLOCK)
#endif
                        done = true;
                    break;
                } else if (count == 0) { /* EOF */
                    done = true;
                    break;
                }

                if (sock && sock->on_read)
                    sock->on_read(sock, buffer, count);
            }
            if (done && sock) {
                list_del(&sock->node);
                sock->close(sock);
            }
        }
    }

    __socket_set_deinit();
    socket->close(socket);
}

void socket_close(struct socket_t *socket)
{
    if (unlikely(!socket))
       return;

    socket->close(socket);
    xfree(socket);
}

