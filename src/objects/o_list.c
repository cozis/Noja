#include <assert.h>
#include "../utils/defs.h"
#include "objects.h"

typedef struct {
	Object base;
	int capacity, count;
	Object **vals;
} ListObject;

static Object *select(Object *self, Object *key, Heap *heap, Error *err);
static _Bool   insert(Object *self, Object *key, Object *val, Heap *heap, Error *err);
static int     count(Object *self);
static void	   print(Object *obj, FILE *fp);
static void    walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp);
static void    walkexts(Object *self, void (*callback)(void   **referer, unsigned int size, void *userp), void *userp);

static TypeObject t_list = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "list",
	.size = sizeof (ListObject),
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

static Object *select(Object *self, Object *key, Heap *heap, Error *error)
{
	assert(self != NULL);
	assert(self->type == &t_list);
	assert(key != NULL);
	assert(heap != NULL);
	assert(error != NULL);

	if(!Object_IsInt(key))
		{
			Error_Report(error, 0, "Non integer key");
			return NULL;
		}

	int idx = Object_ToInt(key, error);
	assert(error->occurred == 0);

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
	assert(list != NULL);

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
	assert(error != NULL);
	assert(key != NULL);
	assert(val != NULL);
	assert(heap != NULL);
	assert(self != NULL);
	assert(self->type == &t_list);

	ListObject *list = (ListObject*) self;

	if(!Object_IsInt(key))
		{
			Error_Report(error, 0, "Non integer key");
			return NULL;
		}

	int idx = Object_ToInt(key, error);
	assert(error->occurred == 0);

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

	for(int i = 0; i < list->count; i += 1)
		{
			Object_Print(list->vals[i], fp);

			if(i+1 < list->count)
				fprintf(fp, ", ");
		}
}