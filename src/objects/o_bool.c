#include <assert.h>
#include <string.h>
#include "objects.h"

static _Bool to_bool(Object *obj, Error *err);
static void print(Object *obj, FILE *fp);

static const Type t_bool = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "bool",
	.size = sizeof (Object),
	.atomic = ATMTP_BOOL,
	.to_bool = to_bool,
	.print = print,
};

static Object the_true_object = {
	.type = &t_bool,
	.flags = Object_STATIC,
};

static Object the_false_object = {
	.type = &t_bool,
	.flags = Object_STATIC,
};

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