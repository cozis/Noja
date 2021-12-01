#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "runtime/runtime.h"
#include "utils/defs.h"

static Object *select_(Object *self, Object *key, Heap *heap, Error *err);
static int      count(Object *self);

static Object *bin_print (Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_count (Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_assert(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_strcat(Runtime *runtime, Object **argv, unsigned int argc, Error *error);

typedef struct {
	Object base;
	Runtime *runtime;
} BuiltinsMapOjbect;

static const Type t_builtins_map = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "builtins map",
	.size = sizeof(BuiltinsMapOjbect),
	.select = select_,
	.count = count,
	.print = NULL,
};

static Object *select_(Object *self, Object *key, Heap *heap, Error *err)
{
	BuiltinsMapOjbect *bm = (BuiltinsMapOjbect*) self;

	if(!Object_IsString(key))
		{
			Error_Report(err, 0, "Non string key");
			return NULL;
		}

	int         n;
	const char *s;

	s = Object_ToString(key, &n, heap, err);

	if(s == NULL)
		return NULL;

	#define PAIR(p, q) \
		(((uint64_t) (p) << 32) | (uint32_t) (q))

	switch(PAIR(n, s[0]))
		{
			case PAIR(sizeof("count")-1, 'c'):
			{
				if(!strcmp(s, "count"))
					return Object_FromNativeFunction(bm->runtime, bin_count, 1, heap, err);
				return NULL;
			}

			case PAIR(sizeof("print")-1, 'p'):
			{
				if(!strcmp(s, "print"))
					return Object_FromNativeFunction(bm->runtime, bin_print, -1, heap, err);
				return NULL;
			}

			case PAIR(sizeof("assert")-1, 'a'):
			{
				if(!strcmp(s, "assert"))
					return Object_FromNativeFunction(bm->runtime, bin_assert, -1, heap, err);
				return NULL;
			}

			case PAIR(sizeof("strcat")-1, 's'):
			{
				if(!strcmp(s, "strcat"))
					return Object_FromNativeFunction(bm->runtime, bin_strcat, -1, heap, err);

				return NULL;
			}
		}

	// Not found.
	return NULL;
}

static int count(Object *self)
{
	(void) self;
	return 3;
}

Object *Object_NewBuiltinsMap(Runtime *runtime, Heap *heap, Error *err)
{
	BuiltinsMapOjbect *bm = (BuiltinsMapOjbect*) Heap_Malloc(heap, &t_builtins_map, err);

	if(bm == NULL)
		return NULL;

	bm->runtime = runtime;
	
	return (Object*) bm;
}

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

static Object *bin_strcat(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	unsigned int total_count = 0;

	for(unsigned int i = 0; i < argc; i += 1)
		{
			if(!Object_IsString(argv[i]))
				{
					Error_Report(error, 0, "Argument #%d is not a string", i+1);
					return NULL;
				}

			total_count += Object_Count(argv[i], error);

			if(error->occurred)
				return NULL;
		}

	char starting[128];
	char *buffer = starting;

	if(total_count > sizeof(starting)-1)
		{
			buffer = malloc(total_count+1);

			if(buffer == NULL)
				{
					Error_Report(error, 1, "No memory");
					return NULL;
				}	
		}

	Object *result = NULL;

	for(unsigned int i = 0, written = 0; i < argc; i += 1)
		{
			int         n;
			const char *s;

			s = Object_ToString(argv[i], &n, Runtime_GetHeap(runtime), error);

			if(error->occurred)
				goto done;

			memcpy(buffer + written, s, n);
			written += n;
		}

	buffer[total_count] = '\0';

	result = Object_FromString(buffer, total_count, Runtime_GetHeap(runtime), error);

done:
	if(starting != buffer)
		free(buffer);
	return result;
}

