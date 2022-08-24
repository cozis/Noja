
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

#include <string.h>
#include "../utils/defs.h"
#include "objects.h"

typedef struct {
	Object base;
	int capacity, count;
	Object **vals;
} ListObject;

static Object *select_(Object *self, Object *key, Heap *heap, Error *err);
static _Bool   insert(Object *self, Object *key, Object *val, Heap *heap, Error *err);
static int     count(Object *self);
static void	   print(Object *obj, FILE *fp);
static void    walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp);
static void    walkexts(Object *self, void (*callback)(void   **referer, unsigned int size, void *userp), void *userp);
static Object *copy(Object *self, Heap *heap, Error *err);
static int hash(Object *self);

static TypeObject t_list = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "list",
	.size = sizeof(ListObject),
	.copy = copy,
	.hash = hash,
	.select = select_,
	.insert = insert,
	.count = count,
	.print = print,
	.walk = walk,
	.walkexts = walkexts,
};

static int hash(Object *self)
{
	ASSERT(self != NULL);
	ASSERT(self->type == &t_list);

	ListObject *ls = (ListObject*) self;

	int h = 0;
	// The hash is the sum of the nested
	// hashes. It's not a smart solution
	// but it works for now.
	for(int i = 0; i < ls->count; i += 1)
		h += Object_Hash(ls->vals[i]);

	return h;
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	UNUSED(heap);
	UNUSED(err);

	ListObject *ls  = (ListObject*) self;
	ListObject *ls2 = (ListObject*) Object_NewList(ls->count, heap, err);
	if(ls2 == NULL) return NULL;

	for(int i = 0; i < ls->count; i += 1)
	{
		ls2->vals[i] = Object_Copy(ls->vals[i], heap, err);
		if(err->occurred) return NULL;
	}

	ls2->count = ls->count;

	return (Object*) ls2;
}

TypeObject *Object_GetListType()
{
	return &t_list;
}

Object *Object_NewList(int capacity, Heap *heap, Error *error)
{
	// Handle default args.
	if(capacity < 8)
		capacity = 8;

	// Make the thing.
	ListObject *obj;
	{
		obj = (ListObject*) Heap_Malloc(heap, &t_list, error);

		if(obj == NULL)
			return NULL;

		obj->count = 0;
		obj->capacity = capacity;
		obj->vals = Heap_RawMalloc(heap, sizeof(Object*) * capacity, error);

		if(obj->vals == NULL)
			return NULL;
	}

	return (Object*) obj;
}

Object *Object_NewList2(int num, Object **items, Heap *heap, Error *error)
{
	ASSERT(num > -1);

	ListObject *list = (ListObject*) Object_NewList(num, heap, error);

	if(list == NULL)
		return NULL;

	memcpy(list->vals, items, num * sizeof(Object*));
	list->count = num;

	return (Object*) list;
}

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp)
{
	ListObject *list = (ListObject*) self;

	for(int i = 0; i < list->count; i += 1)
		callback(&list->vals[i], userp);
}

static void walkexts(Object *self, void (*callback)(void **referer, unsigned int size, void *userp), void *userp)
{
	ListObject *list = (ListObject*) self;
	
	callback((void**) &list->vals, sizeof(Object) * list->capacity, userp);
}

static Object *select_(Object *self, Object *key, Heap *heap, Error *error)
{
	UNUSED(heap);
	ASSERT(self != NULL);
	ASSERT(self->type == &t_list);
	ASSERT(key != NULL);
	ASSERT(heap != NULL);
	ASSERT(error != NULL);

	if(!Object_IsInt(key))
	{
		Error_Report(error, 0, "Non integer key");
		return NULL;
	}

	int idx = Object_GetInt(key);

	ListObject *list = (ListObject*) self;

	if(idx < 0 || idx >= list->count)
	{
		Error_Report(error, 0, "Out of range index");
		return NULL;
	}

	return list->vals[idx];
}

static unsigned int calc_new_capacity(unsigned int old_capacity)
{
	return old_capacity * 2;
}

static _Bool grow(ListObject *list, Heap *heap, Error *error)
{
	ASSERT(list != NULL);

	int new_capacity = calc_new_capacity(list->capacity);

	Object **vals = Heap_RawMalloc(heap, sizeof(Object*) * new_capacity, error);

	if(vals == NULL)
		return 0;

	for(int i = 0; i < list->count; i += 1)
		vals[i] = list->vals[i];

	list->vals = vals;
	list->capacity = new_capacity;
	return 1;
}

static _Bool insert(Object *self, Object *key, Object *val, Heap *heap, Error *error)
{
	ASSERT(error != NULL);
	ASSERT(key != NULL);
	ASSERT(val != NULL);
	ASSERT(heap != NULL);
	ASSERT(self != NULL);
	ASSERT(self->type == &t_list);

	ListObject *list = (ListObject*) self;

	if(!Object_IsInt(key))
	{
		Error_Report(error, 0, "Non integer key");
		return NULL;
	}

	int idx = Object_GetInt(key);

	if(idx < 0 || idx > list->count)
	{
		Error_Report(error, 0, "Out of range index");
		return NULL;
	}

	if(idx == list->count)
	{
		if(list->count == list->capacity)
			if(!grow(list, heap, error))
				return 0;
			
		list->vals[list->count] = val;
		list->count += 1;
	}
	else
	{
		list->vals[idx] = val;
	}

	return 1;
}

static int count(Object *self)
{
	ListObject *list = (ListObject*) self;

	return list->count;
}

static void print(Object *self, FILE *fp)
{
	ListObject *list = (ListObject*) self;

	fprintf(fp, "[");
	for(int i = 0; i < list->count; i += 1)
	{
		Object_Print(list->vals[i], fp);

		if(i+1 < list->count)
			fprintf(fp, ", ");
	}
	fprintf(fp, "]");
}