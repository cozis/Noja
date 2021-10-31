#ifndef PROMISE_H
#define PROMISE_H
#include "bpalloc.h"
typedef struct xPromise Promise;
Promise 	*Promise_New(BPAlloc *alloc, int size);
unsigned int Promise_Size(Promise *promise);
void		 Promise_Free(Promise *promise);
void		 Promise_Resolve(Promise *promise, const void *data, int size);
_Bool 		 Promise_Subscribe(Promise *promise, void *dest);
_Bool 		 Promise_Subscribe2(Promise *promise, void *dest, void *userp, void (*callback)(void*));
#endif