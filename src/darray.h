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
#ifndef __darray_h
#define __darray_h

struct darray {
    void **ptr;  /* the internel array */
    int mem;     /* the sizeof(actually_pointer) */
    int size;
};

#define DEFAULT_SIZE 10
#define SIZE_INCREMENT 2

static inline bool darray_init(struct darray *d, int mem, int size)
{
    /* if size is -1 or 0, use the default */
    if (size <= 0)
        size = DEFAULT_SIZE;
    d->ptr = calloc(size, mem);
    if (!d->ptr)
        return false;
    d->size = size;
    d->mem = mem;
}

static inline void darray_free(struct darray *d, void (*destructor) (void *))
{
    int i;

    for (i = 0; i < d->size; i++)
        if (unlikely(!destructor))
            free(d->ptr[i]);
        else
            (*destructor) (d->ptr[i]);
    free(d->ptr);
    /** XXX it may be a bad idea to this, but just incase
     * the user may want to use the array on something else. */
    d->size = 0;
    d->mem = 0;
}

static inline int darray_put(struct darray *d, void *ptr, void (*constructor) (void *))
{
    void *tmp;
    int place;

    /* Find the first empty place.  */
    for (place = 0; place < d->size && d->ptr[place]; place++);
    /* If there's no place, reallocate  */
    if (place == d->size - 1) {
        tmp = realloc(d->ptr, (d->size + SIZE_INCREMENT) * d->mem);
        if (!tmp)
            return -1;
        d->ptr = tmp;
        d->size += SIZE_INCREMENT;
    }

    d->ptr[place] = ptr;
    if (likely(constructor))
        (*constructor) (ptr);
    return place;
}

static inline bool darray_remove(struct darray *d, void *ptr, bool (*compare_function) (void *, void *),
        void (*destructor) (void *))
{
    int i;
    bool r;

    for (i = 0; i < d->size; i++) {
        if (unlikely(!compare_function)) {
            r = !!(d->ptr[i] == ptr);
            if (r) {
                if (unlikely(!destructor))
                    free(d->ptr[i]);
                else
                    (*destructor) (d->ptr[i]);
                d->ptr[i] = NULL;
            }
        } else {
            r = (*compare_function) (d->ptr[i], ptr);
            if (r) {
                 if (unlikely(!destructor))
                    free(d->ptr[i]);
                else
                    (*destructor) (d->ptr[i]);
                d->ptr[i] = NULL;
            }
        }
    }

    return r;
}

#endif  /* __darray_h */

