
/* +--------------------------------------------------------------------------+
** |                          _   _       _                                   |
** |                         | \ | |     (_)                                  |
** |                         |  \| | ___  _  __ _                             |
** |                         | . ` |/ _ \| |/ _` |                            |
** |                         | |\  | (_) | | (_| |                            |
** |                         |_| \_|\___/| |\__,_|                            |
** |                                    _/ |                                  |
** |                                   |__/                                   |
** +--------------------------------------------------------------------------+
** | Copyright (c) 2022 Francesco Cozzuto <francesco.cozzuto@gmail.com>       |
** +--------------------------------------------------------------------------+
** | This file is part of The Noja Interpreter.                               |
** |                                                                          |
** | The Noja Interpreter is free software: you can redistribute it and/or    |
** | modify it under the terms of the GNU General Public License as published |
** | by the Free Software Foundation, either version 3 of the License, or (at |
** | your option) any later version.                                          |
** |                                                                          |
** | The Noja Interpreter is distributed in the hope that it will be useful,  |
** | but WITHOUT ANY WARRANTY; without even the implied warranty of           |
** | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General |
** | Public License for more details.                                         |
** |                                                                          |
** | You should have received a copy of the GNU General Public License along  |
** | with The Noja Interpreter. If not, see <http://www.gnu.org/licenses/>.   |
** +--------------------------------------------------------------------------+ 
*/

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include "defs.h"
#include "bpalloc.h"

#if USING_VALGRIND
#include <valgrind/memcheck.h>
#endif

#define CHUNK_SIZE 4096
#define PADDING 8

typedef struct BPAllocChunk BPAllocChunk;
struct BPAllocChunk {
	BPAllocChunk *prev;
	char body[];
}; 

struct xBPAlloc {
	int flags;
	void *userp;
	void *(*fn_malloc)(void *userp, int size);
	void  (*fn_free  )(void *userp, void *addr);
	int minsize, size, used;
	BPAllocChunk *tail;
};

enum {
	FG_STATIC = 1,
};

static void *default_fn_malloc(void *userp, int   size);
static void  default_fn_free  (void *userp, void *addr);

BPAlloc *BPAlloc_Init(int chunk_size)
{
	return BPAlloc_Init2(-1, chunk_size, NULL, NULL, NULL);
}

BPAlloc *BPAlloc_Init2(int first_size, int chunk_size, 
						void *userp,
						void *(*fn_malloc)(void *userp, int size), 
						void  (*fn_free  )(void *userp, void *addr))
{
	if(chunk_size < 0)
		chunk_size = CHUNK_SIZE;

	if(first_size < 0)
		first_size = chunk_size;

	if(fn_malloc == NULL)
		{
			userp = NULL;
			fn_malloc = default_fn_malloc;
			fn_free   = default_fn_free;
		}

	void *temp = fn_malloc(userp, sizeof(BPAlloc) + sizeof(BPAllocChunk) + first_size + PADDING);

	if(temp == NULL)
		return NULL;

	BPAlloc *alloc = temp;

	BPAllocChunk *chunk = (BPAllocChunk*) (alloc + 1);
	chunk->prev = NULL;

	alloc->flags 	 = 0; 
	alloc->used 	 = 0;
	alloc->size 	 = first_size;
	alloc->tail 	 = chunk;
	alloc->minsize   = chunk_size;
	alloc->userp     = userp;
	alloc->fn_malloc = fn_malloc;
	alloc->fn_free 	 = fn_free;

#if USING_VALGRIND
	VALGRIND_CREATE_MEMPOOL(alloc, PADDING, 0);
#endif

	return alloc;
}

BPAlloc *BPAlloc_Init3(void *mem, int mem_size, int chunk_size, 
						void *userp,
						void *(*fn_malloc)(void *userp, int size), 
						void  (*fn_free  )(void *userp, void *addr))
{
	assert(mem != NULL);
	assert(mem_size >= 0);

	if(chunk_size < 0)
		chunk_size = CHUNK_SIZE;

	if(fn_malloc == NULL)
		{
			userp = NULL;
			fn_malloc = default_fn_malloc;
			fn_free   = default_fn_free;
		}

	int required = sizeof(BPAlloc) 
				 + sizeof(BPAllocChunk) 
				 + PADDING;

	if(mem_size < required)
		// Not enough memory was provided.
		return NULL;

	BPAlloc *alloc = mem;

	BPAllocChunk *chunk = (BPAllocChunk*) (alloc + 1);
	chunk->prev = NULL;

	alloc->flags	 = FG_STATIC;
	alloc->used 	 = 0;
	alloc->size 	 = mem_size - sizeof(BPAlloc) - sizeof(BPAllocChunk);
	alloc->tail 	 = chunk;
	alloc->minsize   = chunk_size;
	alloc->userp     = userp;
	alloc->fn_malloc = fn_malloc;
	alloc->fn_free 	 = fn_free;

#if USING_VALGRIND
	VALGRIND_CREATE_MEMPOOL(alloc, PADDING, 0);
#endif

	return alloc;
}

void BPAlloc_Free(BPAlloc *alloc)
{
	assert(alloc != NULL);

#if USING_VALGRIND
	VALGRIND_DESTROY_MEMPOOL(alloc);
#endif

	BPAllocChunk *chunk = alloc->tail;

	while(chunk->prev)
		{
			BPAllocChunk *prev = chunk->prev;

			if(alloc->fn_free)
				alloc->fn_free(alloc->userp, chunk);

			chunk = prev;
		}

	if(!(alloc->flags & FG_STATIC))
		if(alloc->fn_free)
			alloc->fn_free(alloc->userp, alloc);
}

void *BPAlloc_Malloc(BPAlloc *alloc, int req_size)
{
	assert(alloc != NULL);
	assert(req_size >= 0);

	alloc->used += PADDING;

	if(alloc->used & 7)
		alloc->used = (alloc->used & ~7) + 8;

	if(alloc->used + req_size > alloc->size)
		{
			// If the chunk size is lower than the
			// requested size, then set the chunk
			// size to the requested size.
			int chunk_size = MAX(alloc->minsize, req_size + PADDING);

			assert(alloc->fn_malloc != NULL);
			BPAllocChunk *chunk = alloc->fn_malloc(alloc->userp, sizeof(BPAllocChunk) + chunk_size);
		
			if(chunk == NULL)
				return NULL;

			chunk->prev = alloc->tail;
			alloc->tail = chunk;
			alloc->size = chunk_size;
			alloc->used = PADDING;

			if(alloc->used & 7)
				alloc->used = (alloc->used & ~7) + 8;
		}

	void *addr = alloc->tail->body + alloc->used;

	assert(((intptr_t) addr) % 8 == 0);
	
	alloc->used += req_size;

#if USING_VALGRIND
	VALGRIND_MEMPOOL_ALLOC(alloc, addr, req_size);
//	VALGRIND_MAKE_MEM_NOACCESS((char*) addr - PADDING, PADDING);
//	VALGRIND_MAKE_MEM_NOACCESS((char*) addr + req_size, PADDING);
#endif

	return addr;
}

static void *default_fn_malloc(void *userp, int size)
{
	assert(userp == NULL);
	assert(size >= 0);
	
	return malloc(size);
}

static void default_fn_free(void *userp, void *addr)
{
	assert(userp == NULL);

	free(addr);
}
