#include <assert.h>
#include "../utils/defs.h"
#include "../objects/objects.h"
#include "runtime.h"

typedef struct {
	Object base;
	Runtime *runtime;
	Executable *exe;
	int index;
} FunctionObject;

static Object *call(Object *self, Object **argv, unsigned int argc, Heap *heap, Error *error);

static const Type t_func = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "function",
	.size = sizeof (FunctionObject),
	.call = call,	
};

static Object *call(Object *self, Object **argv, unsigned int argc, Heap *heap, Error *error)
{
	assert(self != NULL);
	assert(heap != NULL);
	assert(error != NULL);
			
	FunctionObject *func = (FunctionObject*) self;

	assert(func->exe != NULL);
	assert(func->index >= 0);

	return run(func->runtime, error, func->exe, func->index, argv, argc);	
}

Object *Object_FromNojaFunction(Runtime *runtime, Executable *exe, int index, Heap *heap, Error *error)
{
	assert(runtime != NULL);
	assert(exe != NULL);
	assert(index >= 0);
	assert(heap != NULL);
	assert(error != NULL);

	FunctionObject *func = (FunctionObject*) Heap_Malloc(heap, &t_func, error);

	if(func == NULL)
		return NULL;

	func->runtime = runtime;
	func->exe = exe;
	func->index = index;
	return (Object*) func;
}