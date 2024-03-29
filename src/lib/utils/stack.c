
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
#include <stdlib.h>
#include "stack.h"
#include "defs.h"

struct xStack {
	unsigned int size, 
				 used;
	int 		 refs;
	void 		*body[];
};

void *Stack_New(int size)
{
	if(size < 0)
		size = 1024;

	Stack *s = malloc(sizeof(Stack) + sizeof(void*) * size);

	if(s == NULL)
		return NULL;

	ASSERT((intptr_t) s % 8 == 0);

	s->size = size;
	s->used = 0;
	s->refs = 1;
	return s;
}

void *Stack_Top(Stack *s, int n)
{
	ASSERT(n <= 0);

	if(s->used == 0)
		return NULL;

	if((int) s->used + n - 1 < 0)
		return NULL;

	return s->body[s->used + n - 1];
}

void **Stack_TopRef(Stack *s, int n)
{
	ASSERT(n <= 0);

	if(s->used == 0)
		return NULL;

	if((int) s->used + n - 1 < 0)
		return NULL;

	return &s->body[s->used + n - 1];
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
	ASSERT(s != NULL);
	ASSERT(item != NULL);

	if(s->used == s->size)
		return 0;

	s->body[s->used] = item;
	s->used += 1;
	return 1;
}

Stack *Stack_Copy(Stack *s)
{
	s->refs += 1;
	return s;
}

void Stack_Free(Stack *s)
{
	s->refs -= 1;
	ASSERT(s->refs >= 0);
	if(s->refs == 0)
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