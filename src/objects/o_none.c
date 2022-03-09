#include <assert.h>
#include <string.h>
#include "objects.h"

static _Bool op_eql(Object *self, Object *other);
static void   print(Object *obj, FILE *fp);
static int     hash(Object *self);
static Object *copy(Object *self, Heap *heap, Error *err);

static TypeObject t_none = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "none",
	.size = sizeof (Object),
	.hash = hash,
	.copy = copy,
	.print = print,
	.op_eql = op_eql,
};

static Object the_none_object = {
	.type = &t_none,
	.flags = Object_STATIC,
};

static int hash(Object *self)
{
	assert(self == &the_none_object);
	return 0;
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	(void) heap;
	(void) err;
	return self;
}

Object *Object_NewNone(Heap *heap, Error *error)
{
	(void) heap;
	(void) error;
	return &the_none_object;
}

static void print(Object *obj, FILE *fp)
{
	assert(fp != NULL);
	assert(obj != NULL);
	assert(obj->type == &t_none);

	fprintf(fp, "none");
}

static _Bool op_eql(Object *self, Object *other)
{
	(void) self;
	
	assert(other->type == &t_none);
	return 1;
}