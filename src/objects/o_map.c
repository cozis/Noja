
/* +--------------------------------------------------------------------------+
** |                          _   _       _                                   |
** |                         | \ | |     (_)                                  |
** |                         |  \| | ___  _  __ _                             |
** |                         | . ` |/ _ \| |/ _` |                            |
** |                         | |\  | (_) | | (_| |                            |
** |                         |_| \_|\___/| |\__,_|                            |
** |                                    _/ |                                  |
** |                                   |__/                                   |
** +--------------------------------------------------------------------------+
** | Copyright (c) 2022 Francesco Cozzuto <francesco.cozzuto@gmail.com>       |
** +--------------------------------------------------------------------------+
** | This file is part of The Noja Interpreter.                               |
** |                                                                          |
** | The Noja Interpreter is free software: you can redistribute it and/or    |
** | modify it under the terms of the GNU General Public License as published |
** | by the Free Software Foundation, either version 3 of the License, or (at |
** | your option) any later version.                                          |
** |                                                                          |
** | The Noja Interpreter is distributed in the hope that it will be useful,  |
** | but WITHOUT ANY WARRANTY; without even the implied warranty of           |
** | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General |
** | Public License for more details.                                         |
** |                                                                          |
** | You should have received a copy of the GNU General Public License along  |
** | with The Noja Interpreter. If not, see <http://www.gnu.org/licenses/>.   |
** +--------------------------------------------------------------------------+ 
*/

#include <assert.h>
#include "../utils/defs.h"
#include "objects.h"

typedef struct {
	Object base;
	int mapper_size, count;
	int *mapper;
	Object **keys;
	Object **vals;
} MapObject;

static Object *select(Object *self, Object *key, Heap *heap, Error *err);
static _Bool   insert(Object *self, Object *key, Object *val, Heap *heap, Error *err);
static int     count(Object *self);
static void	print(Object *self, FILE *fp);
static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp);
static void walkexts(Object *self, void (*callback)(void **referer, unsigned int size, void *userp), void *userp);
static Object *copy(Object *self, Heap *heap, Error *err);
static int hash(Object *self);

static TypeObject t_map = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "map",
	.size = sizeof (MapObject),
	.copy = copy,
	.hash = hash,
	.select = select,
	.insert = insert,
	.count = count,
	.print = print,
	.walk = walk,
	.walkexts = walkexts,
};

static inline int calc_capacity(int mapper_size)
{
	return mapper_size * 2.0 / 3.0;
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	MapObject *m1 = (MapObject*) self;
	Object *m2 = Object_NewMap(m1->count, heap, err);
	if(m2 == NULL) return NULL;

	for(int i = 0; i < m1->count; i += 1)
		{
			Object *key, *key_cpy;
			Object *val, *val_cpy;

			key = m1->keys[i];
			val = m1->vals[i];

			key_cpy = Object_Copy(key, heap, err);
			if(key_cpy == NULL) return NULL;

			val_cpy = Object_Copy(val, heap, err);
			if(val_cpy == NULL) return NULL;

			if(!Object_Insert(m2, key_cpy, val_cpy, heap, err))
				return NULL;
		}

	return (Object*) m2;
}

static int hash(Object *self)
{
	MapObject *m = (MapObject*) self;

	int h = 0;
	// The hash of the map is the sum of the
	// hashes of each key and each item.
	for(int i = 0; i < m->count; i += 1)
			h += Object_Hash(m->keys[i])
			   + Object_Hash(m->vals[i]);
	return h;
}

Object *Object_NewMap(int num, Heap *heap, Error *error)
{
	// Handle default args.
	if(num < 0)
		num = 0;

	// Calculate initial mapper size.
	int mapper_size, capacity;
	{
		mapper_size = 8;
		while(calc_capacity(mapper_size) < num)
			mapper_size <<= 1;

		capacity = calc_capacity(mapper_size);
	}

	// Make the thing.
	MapObject *obj = (MapObject*) Heap_Malloc(heap, &t_map, error);
	{
		if(obj == 0)
			return 0;

		obj->mapper_size = mapper_size;
		obj->count = 0;
		obj->mapper = Heap_RawMalloc(heap, sizeof(int) * mapper_size, error);
		obj->keys   = Heap_RawMalloc(heap, sizeof(Object*) * capacity, error);
		obj->vals   = Heap_RawMalloc(heap, sizeof(Object*) * capacity, error);

		if(obj->mapper == NULL || obj->keys == NULL || obj->vals == NULL)
			return NULL;
	}

	for(int i = 0; i < mapper_size; i += 1)
		obj->mapper[i] = -1;

	return (Object*) obj;
}

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp)
{
	MapObject *map = (MapObject*) self;

	for(int i = 0; i < map->count; i += 1)
		{
			callback(&map->keys[i], userp);
			callback(&map->vals[i], userp);
		}
}

static void walkexts(Object *self, void (*callback)(void **referer, unsigned int size, void *userp), void *userp)
{
	assert(self->type == &t_map);

	MapObject *map = (MapObject*) self;

	int capacity = calc_capacity(map->mapper_size);
	
	callback((void**) &map->mapper, sizeof(int) * map->mapper_size, userp);
	callback((void**) &map->keys, sizeof(Object*) * capacity, userp);
	callback((void**) &map->vals, sizeof(Object*) * capacity, userp);
}

static Object *select(Object *self, Object *key, Heap *heap, Error *error)
{
	assert(self != NULL);
	assert(self->type == &t_map);
	assert(key != NULL);
	assert(heap != NULL);
	assert(error != NULL);

	MapObject *map = (MapObject*) self;

	unsigned int mask = map->mapper_size - 1;
	unsigned int hash = Object_Hash(key);
	unsigned int pert = hash;

	int i = hash & mask;

	while(1)
		{
			int k = map->mapper[i];

			if(k == -1)
				{
					// Empty slot. 
					// This key is not present.
					return NULL;
				}
			else
				{
					// Found an item. 
					// Is it the right one?
					
					assert(k >= 0);

					if(Object_Compare(key, map->keys[k], error))
						// Found it!
						return map->vals[k];

					if(error->occurred)
						// Key doesn't implement compare.
						return 0;

					// Not the one we wanted.
				}

			pert >>= 5;
			i = (i * 5 + pert + 1) & mask;
		}

	UNREACHABLE;
	return NULL;
}

static _Bool grow(MapObject *map, Heap *heap, Error *error)
{
	assert(map != NULL);

	int new_mapper_size = map->mapper_size << 1;
	int new_capacity = calc_capacity(new_mapper_size);

	int *mapper   = Heap_RawMalloc(heap, sizeof(int) * new_mapper_size, error);
	Object **keys = Heap_RawMalloc(heap, sizeof(Object*) * new_capacity, error);
	Object **vals = Heap_RawMalloc(heap, sizeof(Object*) * new_capacity, error);

	if(mapper == NULL || keys == NULL || vals == NULL)
		return 0;

	for(int i = 0; i < map->count; i += 1)
		{
			keys[i] = map->keys[i];
			vals[i] = map->vals[i];
		}

	for(int i = 0; i < new_mapper_size; i += 1)
		mapper[i] = -1;

	// Rehash everything.
	for(int i = 0; i < map->count; i += 1)
		{
			// This won't trigger an error because the key
			// surely has a hash method since we already
			// hashed it once.
			int hash = Object_Hash(keys[i]);

			int mask = new_mapper_size - 1;
			int pert = hash;
			
			int j = hash & mask;

			while(1)
				{
					if(mapper[j] == -1)
						{
							// No collision.
							// Insert here.
							mapper[j] = i;
							break;
						}

					// Collided. Find a new place.
					pert >>= 5;
					j = (j * 5 + pert + 1) & mask;
				}
		}

	// Done.
	map->mapper = mapper;
	map->mapper_size = new_mapper_size;
	map->keys = keys;
	map->vals = vals;
	return 1;
}

static _Bool insert(Object *self, Object *key, Object *val, Heap *heap, Error *error)
{
	assert(error != NULL);
	assert(key != NULL);
	assert(val != NULL);
	assert(heap != NULL);
	assert(self != NULL);
	assert(self->type == &t_map);

	MapObject *map = (MapObject*) self;

	if(map->count == calc_capacity(map->mapper_size))
		if(!grow(map, heap, error))
			return 0;

	unsigned int mask = map->mapper_size - 1;
	unsigned int hash = Object_Hash(key);
	unsigned int pert = hash;

	int i = hash & mask;

	while(1)
		{
			int k = map->mapper[i];

			if(k == -1)
				{
					// Empty slot. We can insert it here.
					Object *key_copy = Object_Copy(key, heap, error);

					if(key_copy == NULL)
						return NULL;

					map->mapper[i] = map->count;
					map->keys[map->count] = key_copy;
					map->vals[map->count] = val;
					map->count += 1;
					return 1;
				}
			else
				{
					assert(k >= 0);

					if(Object_Compare(key, map->keys[k], error))
						{
							// Already inserted.
							// Overwrite the value.
							map->vals[k] = val;
							return 1;
						}

					if(error->occurred)
						// Key doesn't implement compare.
						return 0;

					// Collision.
				}

			pert >>= 5;
			i = (i * 5 + pert + 1) & mask;
		}

	UNREACHABLE;
	return 0;
}

static int count(Object *self)
{
	MapObject *map = (MapObject*) self;

	return map->count;
}

static void print(Object *self, FILE *fp)
{
	MapObject *map = (MapObject*) self;

	fprintf(fp, "{");
	for(int i = 0; i < map->count; i += 1)
		{
			Object_Print(map->keys[i], fp);
			fprintf(fp, ": ");
			Object_Print(map->vals[i], fp);

			if(i+1 < map->count)
				fprintf(fp, ", ");
		}
	fprintf(fp, "}");
}