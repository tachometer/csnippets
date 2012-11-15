/*
 * Copyright (c) 2009, 2011 Per Ola Kristensson <pok21@cam.ac.uk>.
 * Copyright (c) 2012 asamy <f.fallen45@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __map_h
#define __map_h

__begin_header

typedef unsigned long (*hash_function) (const char *);
typedef bool (*hash_compare) (const void *v1, const void *v2);

struct pair {
    char *key;
    void *value;
};

struct bucket {
    unsigned int count;    /* Count of pairs.  */
    struct pair *pairs;    /* The pairs array.  */
};

struct map {
    unsigned int count;           /* Count of buckets in this map */
    struct bucket *buckets;       /* Buckets array.  */
    hash_function hash_function;  /* The hash function pointer.  This should be set or
                                     map_new() will initialize it as if this was a string map.  */
    hash_compare  hash_comp;      /* The compare function pointer.  This is used when trying to find
                                     a pair.  */
};

/**
 * Initialize the map, set everything to NULL.
 *
 * Intended to work as if the map is stack allocation.
 * To use a heap allocation, allocate it manually.
 *
 * Then call map_new() on it.
 */
#define MAP_INIT { 1, NULL, NULL, NULL }
/*
 * some helper macros for `pair'
 */
#define PAIR_KEY(pair) (pair)->key
#define PAIR_VALUE(pair, cast) (cast)(pair)->value

/**
 * Allocate map members, buckets, etc.  Set the default hash
 * function if it's not already set, and so on with hash_comp.
 */
extern void map_new(struct map *map);
/**
 * Free stuff in `map'.  buckets and so on.
 */
extern void map_free(struct map *map);
/**
 * Get a pair in the map, by key.
 *
 * So why does this return `pair' and not just the value?  Since the hash_comp()
 * function is configurable, this is left to the user just incase if the user would
 * compare with wildcards but then we would return the actual 'key'.
 */
extern struct pair *map_get(const struct map *map, const char *key);
/**
 * Check if the map has key `key'. */
extern bool map_has(const struct map *map, const char *key);
/**
 * Remove `key' from the map.
 *
 * NOTE: This does NOT free the value!
 * returns true on success, false otherwise.
 */
extern bool map_remove(const struct map* map, const char *key);
/**
 * Put a pair in the map.
 *
 * @returns the pair that has been put.  This can be useful in rare cases but it can be
 * ignored.
 */
extern struct pair *map_put(const struct map *map, const char *key, void *value);
/**
 * Calculate the count of (valid) pairs in the map.
 */
extern int map_get_count(const struct map* map);

__end_header
#endif   /* __map_h */

