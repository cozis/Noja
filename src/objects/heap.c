#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "objects.h"

#if USING_VALGRIND
#include <valgrind/memcheck.h>
#endif

typedef struct OflowAlloc OflowAlloc;
struct OflowAlloc {
	OflowAlloc *prev;
	char body[];
};

struct xHeap {
	int   size;
	int   used;
	int   total;
	void *body;
	OflowAlloc *oflow;

	_Bool collecting;
	_Bool collection_failed;
	void *old_body;
	int   old_used;
	int   old_total;
	OflowAlloc *old_oflow;
	Error *error;
};

Heap *Heap_New(int size)
{
	if(size < 0)
		size = 65536;

	Heap *heap = malloc(sizeof(Heap));

	if(heap == NULL)
		return NULL;

	heap->total = 0;
	heap->size = size;
	heap->used = 0;
	heap->body = malloc(size);
	heap->oflow = 0;
	heap->collecting = 0;

	if(heap->body == NULL)
		{
			free(heap);
			return NULL;
		}

#if USING_VALGRIND
	VALGRIND_CREATE_MEMPOOL(heap, 0, 0);
#endif

	return heap;
}

void Heap_Free(Heap *heap)
{

#if USING_VALGRIND
	VALGRIND_DESTROY_MEMPOOL(heap);
#endif

	while(heap->oflow)
		{
			OflowAlloc *prev = heap->oflow->prev;
			free(heap->oflow);
			heap->oflow = prev;
		}

	free(heap->body);
	free(heap);
}

float Heap_GetUsagePercentage(Heap *heap)
{
	return 100.0 * heap->total / heap->size;
}

void *Heap_Malloc(Heap *heap, TypeObject *type, Error *err)
{
	void *addr = Heap_RawMalloc(heap, type->size, err);

	if(addr == NULL)
		return NULL;

	Object *obj = addr;

	obj->type = type;
	obj->flags = 0;

	if(type->init && !type->init(obj, err))
		return NULL;

	obj->type = type;
	obj->flags = 0;

	return (Object*) addr;
}

void *Heap_RawMalloc(Heap *heap, int size, Error *err)
{
	assert(err);
	assert(heap);
	assert(size > -1);

	void *addr;

	if(heap->used + size >= heap->size)
		{
			OflowAlloc *oflow = malloc(sizeof(OflowAlloc) + size);

			if(oflow == 0)
				return 0;

			oflow->prev = heap->oflow;
			heap->oflow = oflow;

			addr = oflow->body;

			heap->total += size;
		}
	else
		{
			int prev_used = heap->used;

			if(heap->used & 7)
				heap->used = (heap->used & ~7) + 8;

			addr = heap->body + heap->used;

			heap->used += size;
			heap->total += heap->used - prev_used;
		}

	assert(((intptr_t) addr) % 8 == 0);

#if USING_VALGRIND
	VALGRIND_MEMPOOL_ALLOC(heap, addr, size);
#endif

	return addr;
}

_Bool Heap_StartCollection(Heap *heap, Error *error)
{
	assert(heap->collecting == 0);

	void *new_body = malloc(heap->size);

	if(new_body == NULL)
		{
			Error_Report(error, 1, "No memory");
			return 0;
		}

	heap->old_body = heap->body;
	heap->old_used = heap->used;
	heap->old_total = heap->total;
	heap->old_oflow = heap->oflow;
	heap->total = 0;
	heap->body = new_body;
	heap->used = 0;
	heap->oflow = NULL;
	heap->collecting = 1;
	heap->collection_failed = 0;
	heap->error = error;
	return 1;
}

_Bool Heap_StopCollection(Heap *heap)
{
	assert(heap->collecting == 1);

	if(heap->collection_failed)
		{
			free(heap->old_body);
			return 0;
		}

	/* Call destructors here */

	while(heap->old_oflow)
		{
			OflowAlloc *prev = heap->old_oflow->prev;
			free(heap->old_oflow);
			heap->old_oflow = prev;
		}

	free(heap->old_body);

	heap->collecting = 0;
	return 1;
}

void Heap_CollectExtension(void **referer, int size, Heap *heap)
{
	assert(referer != NULL);
	assert(heap->collecting);

	void *old_location = *referer;

	if(heap->collection_failed || old_location == NULL)
		return;

	void *new_location = Heap_RawMalloc(heap, size, heap->error);

	if(new_location == NULL)
		{
			heap->collection_failed = 1;
			return;
		}

	memcpy(new_location, old_location, size);

	*referer = new_location;
}

void Heap_CollectReference(Object **referer, Heap *heap)
{
	assert(referer != NULL);
	assert(heap->collecting);

	Object *old_location = *referer;

	if(heap->collection_failed || old_location == NULL)
		return;

	Object *new_location;

	if(old_location->flags & Object_STATIC)
	
		// The object doesn't need to be moved
		// since it was statically allocated.
		new_location = old_location;

	else if(old_location->flags & Object_MOVED)
	
		// The object was already moved.
		new_location = ((MovedObject*) old_location)->new_location;

	else
		{
			// This object wasn't moved to
			// the new heap yet.

			// Get some information.
			TypeObject *type = old_location->type;
			int size         = type->size;

			// Copy the object to a new location.
			{
				new_location = Heap_RawMalloc(heap, size, heap->error);

				if(new_location == NULL)
					{
						heap->collection_failed = 1;
						return;
					}

				memcpy(new_location, old_location, size);
			}

			// Set the old location as moved and
			// leave the reference to the new
			// location.
			{
				old_location->flags |= Object_MOVED;

				assert((int) sizeof(MovedObject) <= size);
				((MovedObject*) old_location)->new_location = new_location;
			}
		}

	// Update the referer
	*referer = new_location;

	// Collect the reference to the type.
	if((Object*) new_location->type != new_location)
		Heap_CollectReference((Object**) &new_location->type, heap);

	// Collect all of the references to
	// extensions allocate using the GC'd
	// heap.
	Object_WalkExtensions(new_location, Heap_CollectExtension, heap);

	// Now collect all of the children.
	Object_WalkReferences(new_location, Heap_CollectReference, heap);
}