
/* Copyright (c) 2022 Francesco Cozzuto <francesco.cozzuto@gmail.com>
**
** This file is part of The Noja Interpreter.
**
** The Noja Interpreter is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at 
** your option) any later version.
**
** The Noja Interpreter is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of 
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General 
** Public License for more details.
**
** You should have received a copy of the GNU General Public License along 
** with The Noja Interpreter. If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <string.h>
#include "promise.h"
#include "defs.h"

typedef struct Gap Gap;
struct Gap {
	Gap  *next;
	void *dest;
	void *userp;
	void (*callback)(void*);
};

struct xPromise {
	BPAlloc *alloc;
	_Bool set;
	Gap *gaps;
	int  size;
	char body[];
};

Promise *Promise_New(BPAlloc *alloc, int size)
{
	assert(alloc != NULL);
	assert(size >= 0);

	Promise *promise = BPAlloc_Malloc(alloc, sizeof(Promise) + size);

	if(promise == NULL)
		return NULL;

	promise->alloc = alloc;
	promise->set = 0;
	promise->gaps = NULL;
	promise->size = size;
	return promise;
}

unsigned int Promise_Size(Promise *promise)
{
	return promise->size;
}

void Promise_Free(Promise *promise)
{
	assert(promise->set == 1);
}

void Promise_Resolve(Promise *promise, const void *data, int size)
{
	assert(size >= 0);
	assert(size == promise->size);
	assert(promise->set == 0);

	memcpy(promise->body, data, size);
	promise->set = 1;

	Gap *gap = promise->gaps;
	while(gap)
		{
			memcpy(gap->dest, data, size);

			if(gap->callback)
				gap->callback(gap->userp);

			gap = gap->next;
		}

	promise->gaps = NULL;
}

_Bool Promise_Subscribe(Promise *promise, void *dest)
{
	assert(promise != NULL);
	assert(dest != NULL);
	return Promise_Subscribe2(promise, dest, NULL, NULL);
}

_Bool Promise_Subscribe2(Promise *promise, void *dest, void *userp, void (*callback)(void*))
{
	assert(promise != NULL);
	assert(dest != NULL);

	if(promise->set == 0)
		{
			Gap *gap = BPAlloc_Malloc(promise->alloc, sizeof(Gap));

			if(gap == NULL)
				return 0;

			gap->next = promise->gaps;
			gap->dest = dest;
			gap->userp = userp;
			gap->callback = callback;
			promise->gaps = gap;
		}
	else
		{
			memcpy(dest, promise->body, promise->size);

			if(callback)
				callback(userp);
		}

	return 1;
}