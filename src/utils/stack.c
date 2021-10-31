#include <stdlib.h>
#include "stack.h"
#include "defs.h"

struct xStack {
	unsigned int size, used;
	void *body[];
};

void *Stack_New(int size)
{
	if(size < 0)
		size = 1024;

	Stack *s = malloc(sizeof(Stack) + sizeof(void*) * size);

	if(s == NULL)
		return NULL;

	s->size = size;
	s->used = 0;
	return s;
}

void *Stack_Top(Stack *s, int n)
{
	assert(n <= 0);

	if(s->used == 0)
		return NULL;

	if((int) s->used + n - 1 < 0)
		return NULL;

	return s->body[s->used + n - 1];
}

_Bool Stack_Pop(Stack *s, unsigned int n)
{
	if(s->used < n)
		return 0;

	s->used -= n;
	return 1;
}

_Bool Stack_Push(Stack *s, void *item)
{
	assert(s != NULL);
	assert(item != NULL);

	if(s->used == s->size)
		return 0;

	s->body[s->used] = item;
	s->used += 1;
	return 1;
}

void Stack_Free(Stack *s)
{
	free(s);
}

unsigned int Stack_Size(Stack *s)
{
	return s->used;
}

unsigned int Stack_Capacity(Stack *s)
{
	return s->size;
}