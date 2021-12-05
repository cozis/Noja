#include <assert.h>
#include <string.h>
#include "objects.h"

static long long int to_int(Object *obj, Error *err);
static void print(Object *obj, FILE *fp);
static _Bool op_eql(Object *self, Object *other);

typedef struct {
	Object base;
	long long int val;
} IntObject;

static TypeObject t_int = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "int",
	.size = sizeof (IntObject),
	.atomic = ATMTP_INT,
	.to_int = to_int,
	.print = print,
	.op_eql = op_eql,
};

static long long int to_int(Object *obj, Error *err)
{
	assert(obj != NULL);
	assert(err != NULL);
	assert(Object_GetType(obj) == &t_int);

	(void) err;

	return ((IntObject*) obj)->val;
}

Object *Object_FromInt(long long int val, Heap *heap, Error *error)
{
	assert(heap != NULL);
	assert(error != NULL);

	IntObject *obj = (IntObject*) Heap_Malloc(heap, &t_int, error);

	if(obj == 0)
		return 0;

	obj->val = val;

	return (Object*) obj;
}

static void print(Object *obj, FILE *fp)
{
	assert(fp != NULL);
	assert(obj != NULL);
	assert(obj->type == &t_int);

	fprintf(fp, "%lld", ((IntObject*) obj)->val);
}

static _Bool op_eql(Object *self, Object *other)
{
	assert(self != NULL);
	assert(self->type == &t_int);
	assert(other != NULL);
	assert(other->type == &t_int);

	IntObject *i1, *i2;

	i1 = (IntObject*) self;
	i2 = (IntObject*) other;

	return i1->val == i2->val;
}