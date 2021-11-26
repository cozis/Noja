#include <assert.h>
#include "runtime.h"

static Object *bin_print(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	for(int i = 0; i < (int) argc; i += 1)
		Object_Print(argv[i], stdout);

	return Object_NewNone(Runtime_GetHeap(runtime), error);
}

static Object *bin_count(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	int n = Object_Count(argv[0], error);

	if(error->occurred)
		return NULL;

	return Object_FromInt(n, Runtime_GetHeap(runtime), error);
}

static Object *bin_assert(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	if(Object_ToBool(argv[0], error))
		{
			return Object_NewNone(Runtime_GetHeap(runtime), error);
		}
	else
		{
			if(!error->occurred)
				Error_Report(error, 0, "Assertion failed");
			return NULL;
		}
}

typedef struct {
	const char *name;
	int 		argc;
	Object *(*callback)(Runtime*, Object**, unsigned int, Error*);
} BuiltinNativeFunctions;

BuiltinNativeFunctions builtins[] = {
	{"print",  -1, bin_print},
	{"count",   1, bin_count},
	{"assert",  1, bin_assert},
};

_Bool add_builtins(Runtime *runtime, Error *error)
{
	Heap *heap = Runtime_GetHeap(runtime);
	assert(heap != NULL);

	Object *dest = Runtime_GetBuiltins(runtime, error);

	if(dest == NULL)
		return 0;

	for(int i = 0; (unsigned int) i < sizeof(builtins) / sizeof(builtins[0]); i += 1)
		{
			Object *name = Object_FromString(builtins[i].name, -1, heap, error);

			if(name == NULL)
				return 0;

			Object *func = Object_FromNativeFunction(runtime, builtins[i].callback, builtins[i].argc, heap, error);
		
			if(func == NULL)
				return NULL;
		
			if(!Object_Insert(dest, name, func, heap, error))
				return 0;
		}

	return 1;
}