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
    unsigned int count;
    struct pair *pairs;
};

struct map {
    unsigned int count;
    struct bucket *buckets;
    hash_function hash_function;
    hash_compare  hash_comp;
};

#define MAP_INIT { 1, NULL, NULL, NULL }
/*
 * some helper macros for `pair'
 */
#define PAIR_KEY(pair) (pair)->key
#define PAIR_VALUE(pair, cast) (cast)(pair)->value

extern void map_new(struct map *map);
extern void map_free(struct map *map);

extern struct pair *map_get(const struct map *map, const char *key);
extern bool map_has(const struct map *map, const char *key);
extern bool map_remove(const struct map* map, const char *key);
extern struct pair *map_put(const struct map *map, const char *key, void *value);
extern int map_get_count(const struct map* map);

__end_header
#endif   /* __map_h */

