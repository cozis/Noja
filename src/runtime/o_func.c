#include <assert.h>
#include <stdlib.h>
#include "../utils/defs.h"
#include "../objects/objects.h"
#include "runtime.h"

typedef struct {
	Object base;
	Runtime *runtime;
	Executable *exe;
	int index, argc;
	Object *closure;
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
	assert(func->argc >= 0);
	assert(func->index >= 0);

	// Make sure the right amount of arguments is provided.

	Object **argv2;

	int expected_argc = func->argc;

	if(expected_argc < (int) argc)
		{
			// Nothing to be done. By using
			// the right argc the additional
			// arguments are ignored implicitly.
			argv2 = argv;
		}
	else if(expected_argc > (int) argc)
		{
			// Some arguments are missing.
			argv2 = malloc(sizeof(Object*) * expected_argc);

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
						return 0;
				}
		}
	else
		// The right amount of arguments was provided.
		argv2 = argv;

	Object *result = run(func->runtime, error, func->exe, func->index, func->closure, argv2, expected_argc);

	if(argv2 != argv)
		free(argv2);

	return result;
}

Object *Object_FromNojaFunction(Runtime *runtime, Executable *exe, int index, int argc, Object *closure, Heap *heap, Error *error)
{
	assert(runtime != NULL);
	assert(exe != NULL);
	assert(index >= 0);
	assert(argc >= 0);
	assert(heap != NULL);
	assert(error != NULL);

	FunctionObject *func = (FunctionObject*) Heap_Malloc(heap, &t_func, error);

	if(func == NULL)
		return NULL;

	Executable *exe_copy = Executable_Copy(exe);

	if(exe_copy == NULL)
		{
			Error_Report(error, 1, "Failed to copy executable");
			return NULL;
		}

	func->runtime = runtime;
	func->exe = exe_copy;
	func->index = index;
	func->argc = argc;
	func->closure = closure;

	return (Object*) func;
}