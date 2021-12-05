#include <assert.h>
#include <stdlib.h>
#include "../utils/defs.h"
#include "../objects/objects.h"
#include "runtime.h"

typedef struct {
	Object base;
	Runtime *runtime;
	Object *(*callback)(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
	int argc;
} NativeFunctionObject;

static Object *call(Object *self, Object **argv, unsigned int argc, Heap *heap, Error *error);

static TypeObject t_nfunc = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "native function",
	.size = sizeof (NativeFunctionObject),
	.call = call,	
};

static Object *call(Object *self, Object **argv, unsigned int argc, Heap *heap, Error *error)
{
	assert(self != NULL);
	assert(heap != NULL);
	assert(error != NULL);
			
	NativeFunctionObject *func = (NativeFunctionObject*) self;

	// If the function isn't variadic, make sure
	// the right amount of arguments is provided.

	Object **argv2;
	int 	 argc2;

	int expected_argc = func->argc;

	if(expected_argc < 0 || expected_argc == (int) argc)
		{
			// The function is variadic or the right 
			// amount of arguments was provided.
			argv2 = argv;
			argc2 = argc;
		}
	else if(expected_argc < (int) argc)
		{
			// Nothing to be done. By using
			// the right argc the additional
			// arguments are ignored implicitly.
			argv2 = argv;
			argc2 = expected_argc;
		}
	else if(expected_argc > (int) argc)
		{
			// Some arguments are missing.
			argv2 = malloc(sizeof(Object*) * expected_argc);
			argc2 = expected_argc;
			
			if(argv2 == NULL)
				{
					Error_Report(error, 1, "No memory");
					return NULL;
				}

			// Copy the provided arguments.
			for(int i = 0; i < (int) argc; i += 1)
				argv2[i] = argv[i];

			// Set the unspecified arguments to none.
			for(int i = argc; i < expected_argc; i += 1)
				{
					argv2[i] = Object_NewNone(heap, error);

					if(argv2[i] == NULL)
						{
							free(argv2);
							return NULL;
						}
				}
		}
	else UNREACHABLE;

	assert(func->callback != NULL);
	Object *result = func->callback(func->runtime, argv2, argc2, error);

	if(result == NULL && error->occurred == 0)
		Error_Report(error, 1, "Native callback returned NULL but didn't report errors");

	if(argv2 != argv)
		free(argv2);

	return result;
}

Object *Object_FromNativeFunction(Runtime *runtime, Object *(*callback)(Runtime*, Object**, unsigned int, Error*), int argc, Heap *heap, Error *error)
{
	assert(callback != NULL);
	assert(heap != NULL);
	assert(error != NULL);

	NativeFunctionObject *func = (NativeFunctionObject*) Heap_Malloc(heap, &t_nfunc, error);

	if(func == NULL)
		return NULL;

	func->runtime = runtime;
	func->callback = callback;
	func->argc = argc;

	return (Object*) func;
}