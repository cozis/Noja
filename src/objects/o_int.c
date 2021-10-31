#include <assert.h>
#include <string.h>
#include "objects.h"

static long long int to_int(Object *obj, Error *err);

typedef struct {
	Object base;
	long long int val;
} IntObject;

static const Type t_int = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "int",
	.size = sizeof (IntObject),
	.atomic = ATMTP_INT,
	.to_int = to_int,
};

static long long int to_int(Object *obj, Error *err)
{
	assert(obj);
	assert(err);
	assert(Object_GetType(obj) == &t_int);

	(void) err;

	return ((IntObject*) obj)->val;
}

Object *Object_FromInt(long long int val, Heap *heap, Error *error)
{
	IntObject *obj = (IntObject*) Heap_Malloc(heap, &t_int, error);

	if(obj == 0)
		return 0;

	obj->val = val;

	return (Object*) obj;
}