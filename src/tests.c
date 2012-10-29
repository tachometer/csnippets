#include "config.h"
#include "asprintf.h"
#include "map.h"
#include "event.h"

#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

static void *test(void *a)
{
    log("test(): %d\n", (int)a);
    return NULL;
}

char *prog;
int main(int argc, char **argv)
{
    prog = argv[0];
    log_init();
#ifdef __hash_map_test
    char *s;
    struct map map = MAP_INIT;
    map_init(&map);
    if (asprintf(&s, "%d %s", argc, prog) < 0)
        fatal("asprintf() failed: %d %s\n", argc, prog);

    log("map count: %d\n", map_get_count(&map));
    struct pair *pair = map_put(&map, "hello", (void *)"world");
    assert(map_has(&map, "hello"));
    log("map count: %d\n", map_get_count(&map));

    warning("%s (pair --> %s %s)\n", s, PAIR_KEY(pair), PAIR_VALUE(pair, char *));;
    assert(map_remove(&map, "hello"));
    assert(!map_has(&map, "hello"));
    log("map count: %d\n", map_get_count(&map));
#else
    events_init();
    event_add(2, test, (void *)2);
    event_add(2, test, (void *)1);
    sleep(5);   /* since the thread is not joinable, wait some time until it executes those events....  */
#endif
    fatal("we're done with the tests!\n");
}

