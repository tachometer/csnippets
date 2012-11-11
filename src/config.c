#include "config.h"
#include "strmisc.h"

struct centry_t *config_parse(const char *filename)
{
    FILE *fp;
    int current_line = 0,
        open_brace   = 0;
    int len, i;
    struct centry_t *ret = NULL;
    char line[2947], **tokens;

    fp = fopen(filename, "r");
    if (!fp)
        fatal("failed to open configuration file %s\n", filename);

    while (fgets(line, 2048, fp)) {
        current_line++;

        char *ptr = strchr(line, '#');
        if (unlikely(ptr))
            *ptr = '\0';
        if (unlikely(*line == '\0' || *line == '\n'))
            continue;
        len = strlen(line);
        switch (open_brace) {
        case 0:
            for (i = 0; i < len; i++) {
                if (line[i] == '{') {
                    open_brace = 1;
                    line[i] = 0;

                    struct centry_t *entry;
                    xmalloc(entry, sizeof(*entry), return NULL);

                    strncpy(entry->section, strtrim(line), 32);

                    entry->section[32] = '\0';
                    entry->def = NULL;

                    if (unlikely(!ret)) {
                        ret = entry;
                        list_head_init(&ret->children);
                    }
                    list_add(&ret->children, &entry->node);
                    break;
                }
            }
            if (unlikely(!open_brace)) {
                fatal("parser error: out of brace at line %d\n",
                        current_line);
            }
            break;
        case 1:
            for (i = 0; i < len; i++) {
                if (line[i] == '}') {
                    open_brace = 0;
                    break;
                }
            }
            if (open_brace) {
                int n_tokens = 0;
                struct cdef_t *def;

                tokens = strexplode(line, '=', &n_tokens);
                if (!tokens || n_tokens > 2) {
                    fatal("parser error: illegal equality at line %d\n",
                            current_line);
                }
 
                xmalloc(def, sizeof(*def), return NULL);
                if (n_tokens == 2) {
                    strncpy(def->key, strtrim(tokens[0]), 33);
                    def->key[32] = '\0';

                    def->value = strdup(strtrim(tokens[1]));
                } else {
                    fatal("parser error: value with no key[?] at line %d\n",
                            current_line);
                }
                if (unlikely(!ret->def)) {
                    ret->def = def;
                    list_head_init(&ret->def->def_children);
                }
                list_add(&ret->def->def_children, &def->node);
            }
            break;
        }
    }

    fclose(fp);
    return ret;
}

