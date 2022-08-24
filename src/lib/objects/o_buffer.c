
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

#include <stdlib.h>
#include <string.h>
#include "objects.h"
#include "../utils/defs.h"
#include "../common/defs.h"

typedef struct {
	size_t refs, size;
	unsigned char body[];
} Payload;

typedef struct {
	Object base;
	Payload *payload;
	size_t offset, length;
} BufferObject;

static Object *buffer_select(Object *self, Object *key, Heap *heap, Error *err);
static _Bool   buffer_insert(Object *self, Object *key, Object *val, Heap *heap, Error *err);
static int     buffer_count(Object *self);
static void	   buffer_print(Object *obj, FILE *fp);
static _Bool   buffer_free(Object *self, Error *error);

static TypeObject t_buffer = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = TYPENAME_BUFFER,
	.size = sizeof(BufferObject),
	.select = buffer_select,
	.insert = buffer_insert,
	.count = buffer_count,
	.print = buffer_print,
	.free  = buffer_free,
};

#define THRESHOLD 128

TypeObject *Object_GetBufferType()
{
	return &t_buffer;
}

_Bool Object_IsBuffer(Object *obj)
{
	return Object_GetType(obj) == Object_GetBufferType();
}

Object *Object_NewBuffer(size_t size, Heap *heap, Error *error)
{
	// Make the thing.
	BufferObject *obj;
	{
		Payload *payload = malloc(sizeof(Payload) + size);
		if(payload == NULL)
		{
			Error_Report(error, 1, "No memory");
			return NULL;
		}
		payload->refs = 1;
		payload->size = size;
		memset(payload->body, 0, size);

		obj = (BufferObject*) Heap_Malloc(heap, &t_buffer, error);
		if(obj == NULL)
			return NULL;

		obj->payload = payload;
		obj->offset = 0;
		obj->length = size;
	}

	return (Object*) obj;
}

static _Bool buffer_free(Object *self, Error *error)
{
	UNUSED(error);

	BufferObject *buffer = (BufferObject*) self;

	Payload *payload = buffer->payload;
	ASSERT(payload != NULL && payload->refs > 0);

	payload->refs -= 1;
	if(payload->refs == 0)
		free(payload);
	return 1;
}

Object *Object_SliceBuffer(Object *obj, size_t offset, size_t length, Heap *heap, Error *error)
{
	if(!Object_IsBuffer(obj))
	{
		Error_Report(error, 0, "Not a " TYPENAME_BUFFER);
		return NULL;
	}

	Payload *payload = ((BufferObject*) obj)->payload;

	if(offset >= payload->size) {
		Error_Report(error, 0, "Offset out of range");
		return NULL;
	}

	if(offset + length > payload->size) {
		Error_Report(error, 0, "Length out of range");
		return NULL;
	}

	BufferObject *slice = (BufferObject*) Heap_Malloc(heap, &t_buffer, error);
	if(slice == NULL)
		return NULL;

	slice->payload = payload;
	slice->offset = offset;
	slice->length = length;

	return (Object*) slice;
}

void *Object_GetBuffer(Object *obj, size_t *size)
{
	if(!Object_IsBuffer(obj))
	{
		Error_Panic("Not a " TYPENAME_BUFFER);
		return NULL;
	}

	BufferObject *buffer = (BufferObject*) obj;
	Payload *payload = buffer->payload;

	if(size) *size = buffer->length;
	return payload->body + buffer->offset;
}

static Object *buffer_select(Object *self, Object *key, Heap *heap, Error *error)
{
	ASSERT(self != NULL);
	ASSERT(self->type == &t_buffer);
	ASSERT(key != NULL);
	ASSERT(heap != NULL);
	ASSERT(error != NULL);

	if(!Object_IsInt(key))
	{
		Error_Report(error, 0, "Non integer key");
		return NULL;
	}

	int idx = Object_GetInt(key);

	BufferObject *buffer = (BufferObject*) self;

	if(idx < 0 || (size_t) idx >= buffer->length)
	{
		Error_Report(error, 0, "Index out of range");
		return NULL;
	}

	Payload *payload = buffer->payload;

	unsigned char byte = payload->body[buffer->offset + idx];

	return Object_FromInt(byte, heap, error);
}

static _Bool buffer_insert(Object *self, Object *key, Object *val, Heap *heap, Error *error)
{
	UNUSED(heap);
	ASSERT(error != NULL);
	ASSERT(key != NULL);
	ASSERT(val != NULL);
	ASSERT(heap != NULL);
	ASSERT(self != NULL);
	ASSERT(self->type == &t_buffer);

	BufferObject *buffer = (BufferObject*) self;

	if(!Object_IsInt(key))
	{
		Error_Report(error, 0, "Non integer key");
		return NULL;
	}
	if(!Object_IsInt(val))
	{
		Error_Report(error, 0, "Non integer value");
		return NULL;
	}
	int idx = Object_GetInt(key);
	long long int qword = Object_GetInt(val);
	if(idx < 0 || (size_t) idx >= buffer->length)
	{
		Error_Report(error, 0, "Out of range index");
		return NULL;
	}
	if(qword > 255 || qword < 0)
	{
		Error_Report(error, 0, "Not in range [0, 255]");
		return NULL;
	}

	unsigned char byte = qword & 0xff;

	Payload *payload = buffer->payload;
	payload->body[buffer->offset + idx] = byte;
	return 1;
}

static int buffer_count(Object *self)
{
	BufferObject *buffer = (BufferObject*) self;

	return buffer->length;
}

static void print_bytes(FILE *fp, unsigned char *addr, int size)
{
	fprintf(fp, "[");

	for(int i = 0; i < size; i += 1)
	{
		unsigned char byte, low, high;

		byte = addr[i];
		low  = byte & 0xf;
		high = byte >> 4;

		ASSERT(low  < 16);
		ASSERT(high < 16);

		char c1, c2;

		c1 = low  < 10 ? low  + '0' : low  - 10 + 'A';
		c2 = high < 10 ? high + '0' : high - 10 + 'A';

		fprintf(fp, "%c%c", c2, c1);

		if(i+1 < size)
			fprintf(fp, ", ");
	}
	fprintf(fp, "]");
}

static void buffer_print(Object *self, FILE *fp)
{
	BufferObject *buffer = (BufferObject*) self;
	print_bytes(fp, buffer->payload->body + buffer->offset, buffer->length);
}