/* 
 * Copyright (C) 2012  asamy <f.fallen45@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __stack_h
#define __stack_h

struct stack {
    void **ptr;     /* the internel array */
    size_t mem;     /* the sizeof(actual_pointer) */
    size_t size;
};

#define DEFAULT_SIZE 10
#define SIZE_INCREMENT 2

static inline bool stack_init(struct stack *s, size_t mem, size_t size)
{
    if (!size)
        size = DEFAULT_SIZE;
    s->ptr = calloc(size, mem);
    if (!s->ptr)
        return false;
    s->size = size;
    s->mem = mem;
    return true;
}

static inline void stack_free(struct stack *s, void (*destructor) (void *))
{
    int i;

    for (i = 0; i < s->size; i++)
        if (!s->ptr[i])
            continue;
        if (unlikely(!destructor))
            free(s->ptr[i]);
        else
            (*destructor) (s->ptr[i]);
    free(s->ptr);
    /** XXX it may be a bad idea to do this, but just incase
     * the user wants to use the array on something else. */
    s->size = 0;
    s->mem = 0;
}

static inline bool stack_grow(struct stack *s, int new_size)
{
    void *tmp = realloc(s->ptr, new_size * s->mem);
    if (!tmp)
        return false;
    s->ptr = tmp;
    s->size = new_size;
    return true;
}

static inline int stack_push(struct stack *s, void *ptr, int where, void (*constructor) (void *))
{
    int place = where;

    /* If where is -1, find the place ourselves.  */
    if (place == -1) {
        /* Find the first empty place.  */
        for (place = 0; place < s->size && s->ptr[place]; place++);
        /* If there's no place, reallocate  */
        if (place == s->size && s->ptr[place] != NULL) {
            if (!stack_grow(s, s->size + SIZE_INCREMENT))
                return -1;
        }
    } else {
        assert(place >= 0);
        if (place > s->size) {
            if (!stack_grow(s, s->size + SIZE_INCREMENT))
                return -1;
        }
    }

    s->ptr[place] = ptr;
    if (likely(constructor))
        (*constructor) (ptr);
    return place;
}

static inline void *stack_pop(struct stack *s)
{
    return s ? s->ptr[--s->size] : NULL;
}

static inline void *stack_top(struct stack *s)
{
    return s ? s->ptr[s->size - 1] : NULL;
}

static inline bool stack_remove(struct stack *s, void *ptr, bool (*compare_function) (const void *, const void *),
        void (*destructor) (void *), bool duplicate)
{
    int i;
    bool r;

    for (i = 0; i < s->size; i++) {
        if (unlikely(!compare_function)) {
            r = !!(s->ptr[i] == ptr);
            if (r) {
                if (unlikely(!destructor))
                    free(s->ptr[i]);
                else
                    (*destructor) (s->ptr[i]);
                s->ptr[i] = NULL;
            }
        } else {
            r = (*compare_function) (s->ptr[i], ptr);
            if (r) {
                 if (unlikely(!destructor))
                    free(s->ptr[i]);
                else
                    (*destructor) (s->ptr[i]);
                s->ptr[i] = NULL;
            }
        }

        if (!duplicate && r)
            break;
    }

    return r;
}

#endif  /* __stack_h */

