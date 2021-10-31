#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "objects.h"

typedef struct OflowAlloc OflowAlloc;
struct OflowAlloc {
	OflowAlloc *prev;
	char body[];
};

struct xHeap {
	int   size;
	int   used;
	void *body;
	OflowAlloc *oflow;
};

Heap *Heap_New(int size)
{
	if(size < 0)
		size = 65536;

	Heap *heap = malloc(sizeof(Heap));

	if(heap == NULL)
		return NULL;

	heap->size = size;
	heap->used = 0;
	heap->body = malloc(size);
	heap->oflow = 0;

	if(heap->body == NULL)
		{
			free(heap);
			return NULL;
		}

	return heap;
}

void Heap_Free(Heap *heap)
{
	while(heap->oflow)
		{
			OflowAlloc *prev = heap->oflow->prev;
			free(heap->oflow);
			heap->oflow = prev;
		}

	free(heap->body);
	free(heap);
}

void *Heap_Malloc(Heap *heap, const Type *type, Error *err)
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
		}
	else
		{
			if(heap->used & 7)
				heap->used = (heap->used & ~7) + 8;

			addr = heap->body + heap->used;
			heap->used += size;
		}

	assert(((intptr_t) addr) % 8 == 0);

	return addr;
}