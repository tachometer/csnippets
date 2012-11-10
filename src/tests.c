#include "config.h"
#include "asprintf.h"
#include "map.h"
#include "event.h"
#include "config.h"
#include "socket.h"
#include "strmisc.h"
#include "stack.h"

#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>

char *prog;

static void config_parse_event(void *filename)
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

void test(void *p)
{
    printf("test()\n");
}

static bool stack_compare_string(const char *s1, const char *s2)
{
    return s1 && !strcmp(s1, s2);
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

static void run_networking_test(bool server)
{
    if (!server) {
        connection_t *conn = malloc(sizeof(connection_t));
        if (!conn)
            return;

        conn->on_connect = on_connect;
        printf("connecting to freenode\n");
        socket_connect(conn, "irc.freenode.net", "6667");
    } else {
        socket_t *sock = socket_create();
        if (!sock)
            return;
        sock->on_accept = on_accept;
        printf("Server will bind to port 1337.\n");
        socket_listen(sock, NULL, 1337, 10);
    }
}

static void run_task_test(void)
{
    tasks_init();
    events_init();
    sleep(2);    /* wait for both threads to run */
    tasks_add(task_create(test, NULL));

    events_add(event_create(2, config_parse_event, (void *)"config_test"));
    events_add(event_create(2, test, NULL));
    sleep(5);

    events_add(event_create(3, test, NULL));
    events_stop();
    tasks_stop();
}

static void run_stack_test(void)
{
    struct stack on_stack;
    int i;

    stack_init(&on_stack, sizeof(char *), 5);
    for (i = 0; i < 10; i++) {
        char *p;
        if (asprintf(&p, "%d", i) < 0)
            return;
        stack_push(&on_stack, p, -1, NULL);
    }

    for (i = 0; i < 10; i++) {
        printf("on_stack.ptr[%d]: %s\n",i, (char *)on_stack.ptr[i]);
        stack_remove(&on_stack, on_stack.ptr[i], stack_compare_string, NULL,
            false);
    }

    stack_free(&on_stack, NULL);
}

int main(int argc, char **argv)
{
    prog = argv[0];
    log_init();

    for (argv++; *argv; argv++) {
        if (!strcmp(*argv, "--run-network-test"))
            run_networking_test(*++argv ? true : false);
        else if (!strcmp(*argv, "--run-task-test"))
            run_task_test();
        else if (!strcmp(*argv, "--run-stack-test"))
            run_stack_test();
        else if (!strcmp(*argv, "--run-all-tests")) {
            run_stack_test();
            run_task_test();
            run_networking_test(true);
        } else if (!strcmp(*argv, "--help")) {
            fputs("--run-network-test      if an argument was passed, a server will be bind otherwise connect to freenode.\n",
                    stdout);
            fputs("--run-task-test         takes no arguments, runs few tasks that has a time frame.\n",
                    stdout);
            fputs("--run-stack-test        takse no arguments, pushes few items to a stack\n",
                    stdout);
            fputs("--run-all-tests         run all of the previous tests in the following order:\n"
                    "                           stack_test -> task_test -> network_test\n",
                    stdout);
            return 0;
        } else
            die("unknown option %s.  run %s --help for more information\n",
                    *argv, prog);
    }

    return 0;
}

