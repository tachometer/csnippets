#include "socket.h"

#include <signal.h>
#include <unistd.h>

static socket_t *socket = NULL;  /* global for signal */
char *prog;  /* progran name */

/**
 * Callbacks, self-explained
 */

static void on_read(connection_t *s, const struct sk_buff *buff)
{
    printf("(read)[%d][%d]: %s\n", s->fd, buff->size, buff->data);
}

static void on_write(connection_t *s, const struct sk_buff *buff)
{
    printf("(write)[%d][%d]: %s\n", s->fd, buff->size, buff->data);
}

static void on_disconnect(connection_t *s)
{
    printf("%s disconnected\n", s->ip);
}

static void on_connect(connection_t *s)
{
    s->on_write = on_write;
    s->on_read = on_read;
    s->on_disconnect = on_disconnect;

    socket_write(s, "hi %s\n", s->ip);
}

static void on_accept(socket_t *s, connection_t *n)
{
    n->on_connect = on_connect;
    printf("Accepted connection from %s\n", n->ip);
}

static void __noreturn signal_handle(int sig)
{
    socket_free(socket);
    printf("Terminated\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    prog = argv[0];
    log_init();

    socket = socket_create(on_accept);
    if (!socket)
        return 1;

    if (!socket_listen(socket, argc > 1 ? argv[1] : NULL,
                argc > 2 ? argv[2] : "1337",
                argc > 3 ? atoi(argv[3]) : 10))
        return 1;

    signal(SIGINT, signal_handle);
    signal(SIGTERM, signal_handle);
    while (1) {
        sleep(5);
        printf("%d connections ATM\n", socket->num_connections);
    }

    socket_free(socket);
    return 1;
}

