#include <assert.h>
#include "objects.h"

static double to_float(Object *obj, Error *err);

typedef struct {
	Object base;
	double val;
} FloatObject;

static const Type t_float = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "float",
	.size = sizeof (FloatObject),
	.atomic = ATMTP_FLOAT,
	.to_float = to_float,
};

static double to_float(Object *obj, Error *err)
{
	assert(obj);
	assert(err);
	assert(Object_GetType(obj) == &t_float);

	(void) err;

	return ((FloatObject*) obj)->val;
}

Object *Object_FromFloat(double val, Heap *heap, Error *error)
{
	FloatObject *obj = (FloatObject*) Heap_Malloc(heap, &t_float, error);

	if(obj == 0)
		return 0;

	obj->val = val;

	return (Object*) obj;
}
