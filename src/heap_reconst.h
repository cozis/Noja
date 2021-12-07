#include "runtime/runtime.h"

typedef struct {
	_Bool in_pool;
	Object *parent;
	void *addr;
	int   size;
} MemoryChunk;

typedef struct {
	int        count;
	MemoryChunk chunks[];
} ReconstructedHeap;

ReconstructedHeap *reconstruct_heap(Runtime *runtime);
