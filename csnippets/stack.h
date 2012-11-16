/*
 * Copyright (c) 2012 asamy <f.fallen45@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __stack_h
#define __stack_h

struct stack {
    void **ptr;     /* the internel array */
    size_t size;
};

#define INITIAL_SIZE 10
#define SIZE_INCREMENT 2

/**
 * Initialize stack `s'.  Allocates memory of size `size` if > 0
 * otherwise uses INITIAL_SIZE
 *
 * \sa stack_free().
 */
static inline bool stack_init(struct stack *s, size_t size)
{
    if (!size)
        size = INITIAL_SIZE;
    s->ptr = calloc(size, sizeof(void *));
    if (!s->ptr)
        return false;
    s->size = size;
    return true;
}

/**
 * Free memory used.
 *
 * if destructor is not NULL, this function calls destructor on each pointer
 * that's going to be destroyed  (which means, the user must free it himself).
 *
 * \sa stack_push().
 */
static inline void stack_free(struct stack *s, void (*destructor) (void *))
{
    int i;

    for (i = 0; i < s->size; ++i) {
        if (!s->ptr[i])
            continue;
        if (!destructor)
            free(s->ptr[i]);
        else
            (*destructor) (s->ptr[i]);
    }

    free(s->ptr);
    /** XXX it may be a bad idea to do this, but just incase
     * the user wants to use the array on something else. */
    s->size = 0;
    s->ptr = NULL;
}

/**
 * Preserve some memory of size `new_size'.
 * Does not free previous memory.
 * This is called whenever memory is needed (Internal use).
 */
static inline bool stack_grow(struct stack *s, int new_size)
{
    void *tmp = realloc(s->ptr, new_size * sizeof(void *));
    if (!tmp)
        return false;
    s->ptr = tmp;
    s->size = new_size;
    return true;
}

/**
 * Push item `ptr' on this stack
 *
 * `where' can be -1 if we have to figure out the place ourselves.
 * Specifiying where is good when the user know where to place (saves some cycles).
 *
 * constructor can be NULL if not needed.
 *
 * \returns -1 on failure or pos of where the item is placed.
 * \sa stack_pop(), stack_top(), stack_remove().
 */
static inline int stack_push(struct stack *s, void *ptr, int where, void (*constructor) (void *))
{
    int place = where;

    /* If where is -1, find the place ourselves.  */
    if (place == -1) {
        /* Find the first empty place.  */
        for (place = 0; place < s->size && s->ptr[place]; ++place);
        /* If there's no space left, reallocate  */
        if (place == s->size && s->ptr[place] != NULL) {
            if (!stack_grow(s, s->size + SIZE_INCREMENT))
                return -1;
        }
    } else {
        assert(place >= 0);
        if (place > s->size) {
            if (!stack_grow(s, (place - s->size) + 1))
                return -1;
        }
    }

    s->ptr[place] = ptr;
    if (constructor)
        (*constructor) (ptr);
    return place;
}

/**
 * Pop an item from the top stack.
 *
 * The user must free the pointer himself (and null terminate if possible).
 *
 * \sa stack_top()
 */
static inline void *stack_pop(struct stack *s)
{
    return s ? s->ptr[--s->size] : NULL;
}

/**
 * Get an item off the top of the stack.
 *
 * This keeps the pointer on the stack.
 *
 * \sa stack_remove(), stack_pop()
 */
static inline void *stack_top(struct stack *s)
{
    return s ? s->ptr[s->size - 1] : NULL;
}

/**
 * Remove an item from the stack.
 *
 * If compare_function is specified, it's used instead.
 * This free's the pointer `ptr' unless destructor is specified.
 * duplicate is useful when the user knows the item can be duplicated.
 *
 * \sa stack_push().
 */
static inline bool stack_remove(struct stack *s, void *ptr, bool (*compare_function) (const void *, const void *),
        void (*destructor) (void *), bool duplicate)
{
    int i;
    bool r;

    for (i = 0; i < s->size; ++i) {
        if (!compare_function) {
            r = !!(s->ptr[i] == ptr);
            if (r) {
                if (!destructor)
                    free(s->ptr[i]);
                else
                    (*destructor) (s->ptr[i]);
                s->ptr[i] = NULL;
            }
        } else {
            r = (*compare_function) (s->ptr[i], ptr);
            if (r) {
                 if (!destructor)
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

