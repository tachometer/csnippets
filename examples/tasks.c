#include "task.h"
#include "event.h"
#include "config.h"

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

    sleep(2);    /* wait for both threads to run */
    tasks_add(task_create(test, NULL));

    events_add(event_create(2, config_parse_event,
                argc > 1 ? (void *)argv[1] : (void *)"config_test"));
    events_add(event_create(2, test, NULL));
    sleep(5);

    events_add(event_create(3, test, NULL));
    events_stop();
    tasks_stop();

    printf("Done\n");
    return 0;
}

