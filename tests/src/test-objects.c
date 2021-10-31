#include <assert.h>
#include <stdio.h>
#include "../../src/objects/objects.h"

#define tassert(exp) if(!(exp)) goto done; 
#define tdone done: if(1) 

static _Bool test_int()
{
	_Bool passed = 0;

	Heap *heap = Heap_New(-1);
	assert(heap != NULL);

	Error error;
	Error_Init(&error);

	// Test [Object_FromInt], [Object_ToInt].

	int n = -3; // Just any value.

	Object *obj = Object_FromInt(n, heap, &error);
	
	tassert(obj != 0);
	tassert(error.occurred == 0);

	int k = Object_ToInt(obj, &error);
	
	tassert(k == n);
	tassert(error.occurred == 0);

	passed = 1;

	tdone {
		Error_Free(&error);
		Heap_Free(heap);
		return passed;
	}
}

static _Bool test_float()
{
	_Bool passed = 0;

	Heap *heap = Heap_New(-1);
	assert(heap != NULL);

	Error error;
	Error_Init(&error);

	// Test [Object_FromFloat], [Object_ToFloat].

	double n = -3.4; // Just any value.

	Object *obj = Object_FromFloat(n, heap, &error);

	if(obj == 0)
		goto done;

	double k = Object_ToFloat(obj, &error);

	if(k != n)
		goto done;

	// ...

	passed = 1;
done:
	Error_Free(&error);
	Heap_Free(heap);
	return passed;
}

int main()
{
	typedef _Bool (*test_t)();
	static const test_t tests[] = {
		test_int,
		test_float,
	};

	for(int i = 0; i < (int) (sizeof(tests) / sizeof(tests[0])); i += 1)
		printf("TEST %d: %s\n", i, test_int() ? "Passed" : "Failed");
	
	return 0;
}