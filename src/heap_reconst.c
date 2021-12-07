#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include "heap_reconst.h"

typedef struct {
	Object *parent;
	void   *addr;
	int     size;
} Extension;

typedef struct {
	Object   **objs;
	Extension *exts;
	int ext_size, ext_used;
	int obj_size, obj_used;
} ReconstructionState;

static _Bool append_ext(ReconstructionState *state, Extension ext)
{
	if(state->exts == NULL)
		{
			int n = 8;
			
			state->exts = malloc(n * sizeof(Extension));

			if(state->exts == NULL)
				return 0;

			state->ext_size = n;
			state->ext_used = 0;
		}
	else if(state->ext_size == state->ext_used)
		{
			int factor = 2;
			
			Extension *temp = realloc(state->exts, factor * state->ext_size * sizeof(Extension));
		
			if(temp == NULL)
				return 0;

			state->exts = temp;
			state->ext_size *= factor;
		}

	state->exts[state->ext_used++] = ext;
	return 1;
}

static _Bool contains_ext(ReconstructionState *state, void *addr)
{
	for(int i = 0; i < state->ext_used; i += 1)
		if(state->exts[i].addr == addr)
			return 1;
	return 0;
}

static void exension_walker(void **addr, unsigned int size, void *userp)
{
	ReconstructionState *state = userp;

	if(contains_ext(state, *addr))
		return;

	if(!append_ext(state, (Extension) { .addr = *addr, .size = size, .parent = NULL }))
		{
			assert(0);
		}
}

static _Bool append_obj(ReconstructionState *state, Object *obj)
{
	if(state->objs == NULL)
		{
			int n = 8;
			
			state->objs = malloc(n * sizeof(Object*));

			if(state->objs == NULL)
				return 0;

			state->obj_size = n;
			state->obj_used = 0;
		}
	else if(state->obj_size == state->obj_used)
		{
			int factor = 2;
			
			Object **temp = realloc(state->objs, factor * state->obj_size * sizeof(Extension));
		
			if(temp == NULL)
				return 0;

			state->objs = temp;
			state->obj_size *= factor;
		}

	state->objs[state->obj_used++] = obj;
	return 1;
}

static _Bool contains_obj(ReconstructionState *state, Object *obj)
{
	for(int i = 0; i < state->obj_used; i += 1)
		if(state->objs[i] == obj)
			return 1;
	return 0;
}

static void object_walker(Object **referer, void *userp)
{
	ReconstructionState *state = userp;
	Object *obj = *referer;

	if(obj == NULL)
		return;

	if(!contains_obj(state, obj))
		{
			// Object wasn't already encountered.
			// Store it at index `left`.

			if(!append_obj(state, obj))
				{
					assert(0);
				}

			// Walk over its extensions.
			{
				Object_WalkExtensions(obj, exension_walker, userp);

				if(state->ext_used > 0)
					{
						int i = state->ext_used-1;
						while(i >= 0 && state->exts[i].parent == NULL)
							{
								state->exts[i].parent = obj;
								i -= 1;
							}
					}
			}

			// Walk over its children.
			Object_WalkReferences(obj, object_walker, state);
		}
	else
		{
			// Object was already encountered and
			// it's pointer is stored at index `index`.
		}
}

static int chunk_order(const void *p1, const void *p2)
{
	const MemoryChunk 
		*c1 = p1, 
		*c2 = p2;

	intptr_t l = (intptr_t) c1->addr;
	intptr_t r = (intptr_t) c2->addr;

	return (l > r) - (l < r);
}

ReconstructedHeap *reconstruct_heap(Runtime *runtime)
{
	ReconstructionState state;
	state.exts = NULL;
	state.ext_used = 0;
	state.ext_size = 0;
	state.objs = NULL;
	state.obj_used = 0;
	state.obj_size = 0;

	Object *builtins = Runtime_GetBuiltins(runtime);
	if(builtins != NULL)
		{
			object_walker(&builtins, &state);
			Object_WalkReferences(builtins, object_walker, &state);
		}

	CallStackScanner *scanner = CallStackScanner_New(runtime);
	assert(scanner != NULL);

	Object *locals, *closure;
	while(CallStackScanner_Next(&scanner, &locals, &closure, NULL, NULL))
		{
			if(locals != NULL)
				{
					object_walker(&locals, &state);
					Object_WalkReferences(locals,  object_walker, &state);
				}

			if(closure != NULL)
				{
					object_walker(&closure, &state);
					Object_WalkReferences(closure, object_walker, &state);
				}
		}

	ReconstructedHeap *recheap = malloc(sizeof(ReconstructedHeap) + (state.obj_used + state.ext_used) * sizeof(MemoryChunk));
	if(recheap == NULL)
		{
			free(state.objs);
			free(state.exts);
			return NULL;
		}

	recheap->count = state.obj_used + state.ext_used;

	Heap        *heap      = Runtime_GetHeap(runtime);
	char        *heap_addr = Heap_GetPointer(heap);
	unsigned int heap_size = Heap_GetSize(heap);

	for(int i = 0; i < state.obj_used; i += 1)
		{
			recheap->chunks[i] = (MemoryChunk) {
				.in_pool = (heap_addr <= (char*) state.objs[i] && (char*) state.objs[i] < heap_addr + heap_size),
				.parent = NULL,
				.addr = state.objs[i],
				.size = state.objs[i]->type->size,
			};
		}

	for(int i = 0; i < state.ext_used; i += 1)
		{
			recheap->chunks[state.obj_used + i] = (MemoryChunk) {
				.in_pool = (heap_addr <= (char*) state.exts[i].addr && (char*) state.exts[i].addr < heap_addr + heap_size),
				.parent = state.exts[i].parent,
				.addr   = state.exts[i].addr,
				.size   = state.exts[i].size,
			};
		}

	qsort(recheap->chunks, recheap->count, sizeof(MemoryChunk), chunk_order);

	free(state.objs);
	free(state.exts);
	return recheap;
}