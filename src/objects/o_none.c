#include <assert.h>
#include <string.h>
#include "objects.h"

static const Type t_none = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "none",
	.size = sizeof (Object),
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