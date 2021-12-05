#include <assert.h>
#include <string.h>
#include "objects.h"

static _Bool op_eql(Object *self, Object *other);
static void print(Object *obj, FILE *fp);

static TypeObject t_none = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "none",
	.size = sizeof (Object),
	.print = print,
	.op_eql = op_eql,
};

static Object the_none_object = {
	.type = &t_none,
	.flags = Object_STATIC,
};

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