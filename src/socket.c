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
 */
#include "socket.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <sys/epoll.h>

static void setup_async(const struct socket_t *socket)
{
    int flags;

    flags = fcntl(socket->fd, F_GETFL, 0);
    if (!(flags & O_NONBLOCK))
        flags |= O_NONBLOCK;

    fcntl(socket->fd, F_SETFL, flags);
}

static int __write(void *self, const char *data, int len)
{
    int32_t ret;
    struct socket_t *socket = sock_cast(self);
    if (!socket)
        return -1;

    do
        ret = write(socket->fd, data, len);
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

    socket->fd = -1;
    return 0;
}

static __inline__ void setup_socket(struct socket_t *socket)
{
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

    bcopy((char *)hp->h_addr, (char *)&srv.sin_addr, hp->h_length);
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);

    start = time(NULL);
    while (!connected && time(NULL) - start < 10) {
        errno = 0;
#ifdef __debug_socket
        print("Trying %s...\n", addr);
#endif
        if (connect(socket->fd, (struct sockaddr *)&srv, sizeof(srv)) == -1) {
            switch (errno) {
                case EISCONN:
                case EALREADY:
                case EINPROGRESS:
                    setsockopt(socket->fd, SOL_SOCKET, SO_LINGER,    0, 0);
                    setsockopt(socket->fd, SOL_SOCKET, SO_REUSEADDR, 0, 0);
                    setsockopt(socket->fd, SOL_SOCKET, SO_KEEPALIVE, 0, 0);
                    connected = true;
                    break;
            }
        } else
            connected = true;
    }

#ifdef __debug_socket
    print(connected ? "Successfully connected to %s\n" : "Failed to connect to %s.", addr);
#endif
    return connected;
}

bool socket_listen(struct socket_t *socket, const char *address, int32_t port)
{
    struct sockaddr_in srv;
    int reuse_addr = 1;
    if (!socket)
        return false;
    if (socket->fd < 0)
        return false;

    bzero((char *)&srv, sizeof(srv));
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
    int n_fds, i, epoll_fd;
    struct epoll_event event;
    struct epoll_event *events;

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        log_errno("failed to create epoll fd");
        return;
    }

    event.data.fd = socket->fd;
    event.events  = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket->fd, &event) == -1) {
        log_errno("epoll_ctl failed");
        return;
    }

    xcalloc(events, MAX_EVENTS, sizeof(event), return);
    for (;;) {
        n_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (i = 0; i < n_fds; i++) {
            struct socket_t *sock = NULL;
            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP ||
                    !(events[i].events & EPOLLIN)) {
                warning("epoll error\n");
                close(events[i].data.fd);
            } else if (socket->fd == events[i].data.fd) {
                if (socket->type == STREAM_SERVER) {
                    struct socket_t *new_socket;
                    struct sockaddr_in their_addr;
                    socklen_t len = sizeof(their_addr);
                    int their_fd;

                    their_fd = accept(socket->fd, &their_addr, &len);
                    if (their_fd == -1) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK)
                            log_errno("accept() on fd %d", socket->fd);
                        break;
                    }

                    xmalloc(new_socket, sizeof(*new_socket), break);
                    setup_socket(new_socket);
                    new_socket->fd = their_fd;
                    strncpy(new_socket->ip, inet_ntoa(their_addr.sin_addr), 16);
                    setup_async(new_socket);

                    if (socket->on_accept) {
                        socket->on_accept(socket, new_socket);
                        if (new_socket->on_connect)
                            new_socket->on_connect(new_socket);
                    }
                    list_add(&socket->children, &new_socket->node);

                    event.data.fd = their_fd;
                    event.events = EPOLLIN | EPOLLET;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, their_fd, &event) == -1) {
                        log_errno("epoll_ctl returned -1 ;-(");
                        break;
                    } 
                    continue;
                } else
                    sock = socket;
            }
	    bool done = false;
            if (socket->type == STREAM_SERVER && !list_empty(&socket->children)) {
	        list_for_each(&socket->children, sock, node) {
		    if (sock->fd == events[i].data.fd)
                        break;
                }
            }
            for (;;) {
                ssize_t count;
                char buffer[4096];

                count = read(events[i].data.fd, buffer, sizeof buffer);
                if (count == -1) {
                    if (errno != EAGAIN)
                        done = true;
                    break;
                } else if (count == 0) { /* EOF */
                    done = true;
                    break;
                }

                if (sock && sock->on_read)
                    sock->on_read(sock, buffer, count);
            }
            if (done) {
                close(events[i].data.fd);
                if (sock) {
                    list_del(&sock->node);
                    if (sock->on_disconnect)
                        sock->on_disconnect(sock);
                }
            }
        }
    }

    free(events);
    close(socket->fd);
}

void socket_close(struct socket_t *socket)
{
    if (unlikely(!socket))
       return;

    socket->close(socket);
    xfree(socket);
}

