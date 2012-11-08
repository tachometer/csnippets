#include "pre.h"
#include "config.h"
#include "asprintf.h"
#include "map.h"
#include "event.h"
#include "config.h"
#include "socket.h"
#include "strmisc.h"
#include "darray.h"

#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>

char *prog;

static void *config_parse_event(void *filename)
{
    struct centry_t *c, *p = NULL;
    struct cdef_t *def = NULL;

    c = config_parse((char *)filename);
    if (!c)
        fatal("failed to load config!");

    for (;;) {
        p = list_top(&c->children, struct centry_t, node);
        if (!p)
            break;

        print("Section %s\n", p->section);
        for (;;) {
            def = list_top(&c->def->def_children, struct cdef_t, node);
            if (!def)
                break;

            print("Key [%s] -> Value [%s]\n", def->key, def->value);
            list_del(&def->node);
            free(def);
        }
        list_del(&p->node);
        free(p);
    }
}

static void on_read(connection_t *s, const char *buffer, int len)
{
    printf("[%d]: %s", s->fd, buffer);
}

static void on_disconnect(connection_t *s)
{
    printf("%s disconnected\n", s->ip);
    close(s->fd);
    xfree(s);
}

static void on_connect(connection_t *s)
{
    s->on_read = on_read;
    s->on_disconnect = on_disconnect;

    socket_write(s, "hi %s\n", s->ip);
}

static void on_accept(socket_t *s, connection_t *n)
{
    n->on_connect = on_connect;
    printf("Accepted connection from %s\n", n->ip);
}

static bool darray_compare_string(const char *s1, const char *s2)
{
    return s1 && !strcmp(s1, s2);
}

int main(int argc, char **argv)
{
    prog = argv[0];
    log_init();

    printf("running tests...\n");

    if (argc > 1) {
        char *str = str_convert(argv[1], toupper);
        struct darray on_stack;
        if (!str)
            return 1;
        printf("%s converted -> %s\n", argv[1], str);
        assert(str_cmp(str, isupper));

        darray_init(&on_stack, sizeof(char *), 5);
        darray_put(&on_stack, str, NULL);
        int i;
        for (i = 0; i < 10; i++) {
            char *p;
            if (asprintf(&p, "%d", i) < 0)
                return 1;
            darray_put(&on_stack, p, NULL);
        }

        for (i = 0; i < 10; i++) {
            printf("on_stack.ptr[%d]: %s\n",i, on_stack.ptr[i]);
            darray_remove(&on_stack, on_stack.ptr[i], darray_compare_string, NULL);
        }

        darray_free(&on_stack, NULL);
    }

    char *s;
    struct map map = MAP_INIT;
    map_new(&map);
    if (asprintf(&s, "%d %s", argc, prog) < 0)
        fatal("asprintf() failed: %d %s\n", argc, prog);
    free(s);

    print("map count: %d\n", map_get_count(&map));
    struct pair *pair = map_put(&map, "hello", (void *)"world");
    assert(map_has(&map, "hello"));
    print("map count: %d\n", map_get_count(&map));

    print("%s (pair --> %s %s)\n", s, PAIR_KEY(pair), PAIR_VALUE(pair, char *));;
    assert(map_remove(&map, "hello"));
    assert(!map_has(&map, "hello"));
    print("map count: %d\n", map_get_count(&map));
    map_free(&map);

    events_init();
    event_add(2, config_parse_event, (void *)"config_test");
    events_stop();

    if (argc > 2) {
        connection_t *conn = malloc(sizeof(connection_t));
        conn->on_connect = on_connect;
        socket_connect(conn, argv[1], argv[2]);
    } else {
        socket_t *sock = socket_create();
        if (!sock)
            return 1;
        sock->on_accept = on_accept;
        socket_listen(sock, NULL, 1337, 10);
    }

    return 0;
}

