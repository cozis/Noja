#include <assert.h>
#include <string.h>
#include "objects.h"

static void print(Object *obj, FILE *fp);

static const Type t_none = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "none",
	.size = sizeof (Object),
	.print = print,
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