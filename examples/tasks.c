#include "task.h"
#include "event.h"
#include "config.h"

#include <unistd.h>
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

int main(int argc, char **argv)
{
    prog = argv[0];
    log_init();

    tasks_init();
    events_init();

    printf("Giving some time for the threads to run...\n");
    sleep(2);    /* wait for both threads to run */

    printf("Adding task to test()\n");
    tasks_add(task_create(test, NULL));

    printf("Creating an event for reading configuration.\n");
    events_add(event_create(2, config_parse_event,
                argc > 1 ? (void *)argv[1] : (void *)"config_test"));

    printf("Adding an event (with the test() function as the routine)\n");
    events_add(event_create(2, test, NULL));

    printf("Waiting a bit for the tasks to execute...\n");
    sleep(5);

    printf("Adding another event (with the test() function as the routine)\n");
    events_add(event_create(3, test, NULL));

    printf("Stopping both threads\n");
    events_stop();
    tasks_stop();

    printf("Done\n");
    return 0;
}

