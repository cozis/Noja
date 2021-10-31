#ifndef BPALLOC_H
#define BPALLOC_H
typedef struct xBPAlloc BPAlloc;
BPAlloc*	BPAlloc_Init(int chunk_size);
BPAlloc*	BPAlloc_Init2(int first_size, int chunk_size, void *userp, void *(*fn_malloc)(void *userp, int size), void  (*fn_free  )(void *userp, void *addr));
BPAlloc*	BPAlloc_Init3(void *mem, int mem_size, int chunk_size, void *userp, void *(*fn_malloc)(void *userp, int size), void  (*fn_free  )(void *userp, void *addr));
void 	 	BPAlloc_Free(BPAlloc *alloc);
void*		BPAlloc_Malloc(BPAlloc *alloc, int req_size);
#endif