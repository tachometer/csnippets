#include "stack.h"

char *prog;

static bool stack_compare_string(const char *s1, const char *s2)
{
    return s1 && !strcmp(s1, s2);
}

int main(int argc, char **argv)
{
    prog = argv[0];
    log_init();

    struct stack on_stack;
    int i;

    stack_init(&on_stack, 5);
    for (i = 0; i < argc; i++)
        stack_push(&on_stack, strdup(argv[i]), -1, NULL);

    for (i = 0; i < argc; i++) {
        printf("on_stack.ptr[%d]: %s\n",i, (char *)on_stack.ptr[i]);
        stack_remove(&on_stack, on_stack.ptr[i], stack_compare_string, NULL,
            true);
    }

    stack_free(&on_stack, NULL);

    return 0;
}

