#include <assert.h>
#include <string.h>
#include "objects.h"

static _Bool to_bool(Object *obj, Error *err);
static void print(Object *obj, FILE *fp);
static _Bool op_eql(Object *self, Object *other);
static int hash(Object *self);
static Object *copy(Object *self, Heap *heap, Error *err);

static TypeObject t_bool = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "bool",
	.size = sizeof (Object),
	.atomic = ATMTP_BOOL,
	.hash = hash,
	.copy = copy,
	.to_bool = to_bool,
	.print = print,
	.op_eql = op_eql,
};

static Object the_true_object = {
	.type = &t_bool,
	.flags = Object_STATIC,
};

static Object the_false_object = {
	.type = &t_bool,
	.flags = Object_STATIC,
};

static int hash(Object *self)
{
	assert(self != NULL);
	assert(self->type == &t_bool);

	if(self == &the_true_object)
		return 1;

	assert(self == &the_false_object);
	return 0;
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	(void) heap;
	(void) err;
	return self;
}

static _Bool op_eql(Object *self, Object *other)
{
	assert(self != NULL);
	assert(self->type == &t_bool);
	assert(other != NULL);
	assert(other->type == &t_bool);

	return self == other;
}

static _Bool to_bool(Object *obj, Error *err)
{
	assert(obj);
	assert(err);
	assert(Object_GetType(obj) == &t_bool);
	return obj == &the_true_object;
}

Object *Object_FromBool(_Bool val, Heap *heap, Error *error)
{
	(void) heap;
	(void) error;
	return val ? &the_true_object : &the_false_object;
}

static void print(Object *obj, FILE *fp)
{
	assert(fp != NULL);
	assert(obj != NULL);
	assert(obj->type == &t_bool);

	fprintf(fp, obj == &the_true_object ? "true" : "false");
}