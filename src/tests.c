#include "pre.h"
#include "config.h"
#include "asprintf.h"
#include "map.h"
#include "event.h"
#include "config.h"
#include "socket.h"

#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

char *prog;

static void *test(void *a)
{
    print("test(): %d\n", (int)a);
}

static void *config_parse_event(void *filename)
{
    struct centry_t *c, *p = NULL;
    struct cdef_t *def = NULL;

    c = config_parse((char *)filename);
    if (!c)
        fatal("failed to load config!");

    list_for_each(&c->children, p, node) {
        print("Section %s\n", p->section);
        list_for_each(&p->def->def_children, def, node)
            print("Key [%s] -> Value [%s]\n", def->key, def->value);
    }
}

void on_read(connection_t *s, const char *buffer, int len)
{
    printf("[%d]: %s", s->fd, buffer);
}

void on_disconnect(connection_t *s)
{
    printf("%s disconnected\n", s->ip);
    xfree(s);
}

void on_connect(connection_t *s)
{
    s->on_read = on_read;
    s->on_disconnect = on_disconnect;

    char hello[] = "hi there!\n";
    write(s->fd, hello,sizeof(hello));
}

void on_accept(socket_t *s, connection_t *n)
{
    n->on_connect = on_connect;
    printf("Accepted connection from %s\n", n->ip);
}

int main(int argc, char **argv)
{
    prog = argv[0];
    log_init();
#if 0
    char *s;
    struct map map = MAP_INIT;
    map_init(&map);
    if (asprintf(&s, "%d %s", argc, prog) < 0)
        fatal("asprintf() failed: %d %s\n", argc, prog);

    print("map count: %d\n", map_get_count(&map));
    struct pair *pair = map_put(&map, "hello", (void *)"world");
    assert(map_has(&map, "hello"));
    print("map count: %d\n", map_get_count(&map));

    print("%s (pair --> %s %s)\n", s, PAIR_KEY(pair), PAIR_VALUE(pair, char *));;
    assert(map_remove(&map, "hello"));
    assert(!map_has(&map, "hello"));
    print("map count: %d\n", map_get_count(&map));

    events_init();
    event_add(1, test, (void *)1);
    event_add(2, test, (void *)2);
    event_add(2, config_parse_event, (void *)"config_test");
    events_stop();
#endif

    socket_t *sock = socket_create();
    connection_t *conn = malloc(sizeof(connection_t));
    conn->on_connect=on_connect;
    if (!socket_connect(conn, sock, "irc.freenode.net", "6667"))
        fatal("failed!\n");
    //sock->on_accept = on_accept;
    socket_poll(sock);
    return 0;
}

