
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
#include <assert.h>
#include <string.h>
#include "../utils/defs.h"
#include "objects.h"

typedef struct {
	Object         base;
	int            size;
	unsigned char *body;
} BufferObject;

typedef struct {
	Object        base;
	BufferObject *sliced;
	int           offset, 
	              length;
} BufferSliceObject;

static Object *buffer_select(Object *self, Object *key, Heap *heap, Error *err);
static _Bool   buffer_insert(Object *self, Object *key, Object *val, Heap *heap, Error *err);
static int     buffer_count(Object *self);
static void	   buffer_print(Object *obj, FILE *fp);
static _Bool   buffer_free(Object *self, Error *error);

static Object *slice_select(Object *self, Object *key, Heap *heap, Error *err);
static _Bool   slice_insert(Object *self, Object *key, Object *val, Heap *heap, Error *err);
static int     slice_count(Object *self);
static void	   slice_print(Object *obj, FILE *fp);


static TypeObject t_buffer = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "buffer",
	.size = sizeof (BufferObject),
	.select = buffer_select,
	.insert = buffer_insert,
	.count = buffer_count,
	.print = buffer_print,
	.free  = buffer_free,
};

static TypeObject t_buffer_slice = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "buffer slice",
	.size = sizeof (BufferSliceObject),
	.select = slice_select,
	.insert = slice_insert,
	.count = slice_count,
	.print = slice_print,
};

#define THRESHOLD 128

_Bool Object_IsBuffer(Object *obj)
{
	return obj->type == &t_buffer_slice || obj->type == &t_buffer;
}

Object *Object_NewBuffer(int size, Heap *heap, Error *error)
{
	assert(size >= 0);

	// Make the thing.
	BufferObject *obj;
	{
		obj = (BufferObject*) Heap_Malloc(heap, &t_buffer, error);

		if(obj == NULL)
			return NULL;

		unsigned char *body;

		if(size > THRESHOLD)
		{
			body = malloc(sizeof(unsigned char) * size);

			if(body == NULL)
			{
				Error_Report(error, 1, "No memory");
				return NULL;
			}
		}
		else
		{
			body = Heap_RawMalloc(heap, sizeof(unsigned char) * size, error);
				
			if(body == NULL)
				return NULL;
		}

		obj->size = size;
		obj->body = body;
		memset(obj->body, 0, size);
	}

	return (Object*) obj;
}

static _Bool buffer_free(Object *self, Error *error)
{
	(void) error;

	BufferObject *buffer = (BufferObject*) self;
	
	if(buffer->size > THRESHOLD)
		free(buffer->body);
	return 1;
}

Object *Object_SliceBuffer(Object *buffer, int offset, int length, Heap *heap, Error *error)
{
	if(buffer->type != &t_buffer && buffer->type != &t_buffer_slice)
	{
		Error_Report(error, 0, "Not a buffer or a buffer slice");
		return NULL;
	}

	BufferSliceObject *slice;

	if(buffer->type == &t_buffer)
	{
		BufferObject *original = (BufferObject*) buffer;

		if(offset == 0 && length == original->size)
			return buffer;

		slice = (BufferSliceObject*) Heap_Malloc(heap, &t_buffer_slice, error);

		if(slice == NULL)
			return NULL;

		if(offset < 0 || offset >= original->size)
		{
			Error_Report(error, 0, "offset out of range");
			return NULL;
		}

		if(length < 0 || length >= original->size)
		{
			Error_Report(error, 0, "length out of range");
			return NULL;
		}

		if(offset + length > original->size)
		{
			Error_Report(error, 0, "slice out of range");
			return NULL;
		}

		slice->sliced = original;
		slice->offset = offset;
		slice->length = length;
	}
	else
	{
		assert(buffer->type == &t_buffer_slice);

		slice = (BufferSliceObject*) Heap_Malloc(heap, &t_buffer_slice, error);

		if(slice == NULL)
			return NULL;

		BufferSliceObject *original = (BufferSliceObject*) buffer;

		if(offset < 0 || offset >= original->length)
		{
			Error_Report(error, 0, "offset out of range");
			return NULL;
		}

		if(length < 0 || length >= original->length)
		{
			Error_Report(error, 0, "length out of range");
			return NULL;
		}

		if(offset + length > original->length)
		{
			Error_Report(error, 0, "slice out of range");
			return NULL;
		}

		slice->sliced = original->sliced;
		slice->offset = original->offset + offset;
		slice->length = length;
	}

	return (Object*) slice;
}

void *Object_GetBufferAddrAndSize(Object *obj, int *size, Error *error)
{
	if(obj->type != &t_buffer && obj->type != &t_buffer_slice)
	{
		Error_Report(error, 0, "Not a buffer or a buffer slice");
		return NULL;
	}

	if(obj->type == &t_buffer)
	{
		BufferObject *buffer = (BufferObject*) obj;
		
		if(size)
			*size = buffer->size;

		return buffer->body;
	}
	else
	{
		assert(obj->type == &t_buffer_slice);

		BufferSliceObject *slice = (BufferSliceObject*) obj;

		if(size)
			*size = slice->length;

		return slice->sliced->body + slice->offset;
	}
}

static Object *buffer_select(Object *self, Object *key, Heap *heap, Error *error)
{
	assert(self != NULL);
	assert(self->type == &t_buffer);
	assert(key != NULL);
	assert(heap != NULL);
	assert(error != NULL);

	if(!Object_IsInt(key))
	{
		Error_Report(error, 0, "Non integer key");
		return NULL;
	}

	int idx = Object_ToInt(key, error);
	assert(error->occurred == 0);

	BufferObject *buffer = (BufferObject*) self;

	if(idx < 0 || idx >= buffer->size)
	{
		Error_Report(error, 0, "Out of range index");
		return NULL;
	}

	unsigned char byte = buffer->body[idx];

	return Object_FromInt(byte, heap, error);
}

static Object *slice_select(Object *self, Object *key, Heap *heap, Error *error)
{
	assert(self != NULL);
	assert(self->type == &t_buffer_slice);
	assert(key != NULL);
	assert(heap != NULL);
	assert(error != NULL);

	if(!Object_IsInt(key))
	{
		Error_Report(error, 0, "Non integer key");
		return NULL;
	}

	int idx = Object_ToInt(key, error);
	assert(error->occurred == 0);

	BufferSliceObject *slice = (BufferSliceObject*) self;

	if(idx < 0 || idx >= slice->length)
	{
		Error_Report(error, 0, "Out of range index");
		return NULL;
	}

	unsigned char byte = slice->sliced->body[slice->offset + idx];

	return Object_FromInt(byte, heap, error);
}

static _Bool buffer_insert(Object *self, Object *key, Object *val, Heap *heap, Error *error)
{
	assert(error != NULL);
	assert(key != NULL);
	assert(val != NULL);
	assert(heap != NULL);
	assert(self != NULL);
	assert(self->type == &t_buffer);

	BufferObject *buffer = (BufferObject*) self;

	if(!Object_IsInt(key))
	{
		Error_Report(error, 0, "Non integer key");
		return NULL;
	}

	int idx = Object_ToInt(key, error);
	assert(error->occurred == 0);

	if(idx < 0 || idx >= buffer->size)
	{
		Error_Report(error, 0, "Out of range index");
		return NULL;
	}

	long long int qword = Object_ToInt(val, error);

	if(qword > 255 || qword < 0)
	{
		Error_Report(error, 0, "Not in range [0, 255]");
		return NULL;
	}

	unsigned char byte = qword & 0xff;

	if(error->occurred == 1)
		return 0;

	buffer->body[idx] = byte;
	return 1;
}

static _Bool slice_insert(Object *self, Object *key, Object *val, Heap *heap, Error *error)
{
	assert(error != NULL);
	assert(key != NULL);
	assert(val != NULL);
	assert(heap != NULL);
	assert(self != NULL);
	assert(self->type == &t_buffer_slice);

	BufferSliceObject *slice = (BufferSliceObject*) self;

	if(!Object_IsInt(key))
	{
		Error_Report(error, 0, "Non integer key");
		return NULL;
	}

	int idx = Object_ToInt(key, error);
	assert(error->occurred == 0);

	if(idx < 0 || idx >= slice->length)
	{
		Error_Report(error, 0, "Out of range index");
		return NULL;
	}

	long long int qword = Object_ToInt(val, error);

	if(qword > 255 || qword < 0)
	{
		Error_Report(error, 0, "Not in range [0, 255]");
		return NULL;
	}

	unsigned char byte = qword & 0xff;

	if(error->occurred == 1)
		return 0;

	slice->sliced->body[slice->offset + idx] = byte;
	return 1;
}

static int buffer_count(Object *self)
{
	BufferObject *buffer = (BufferObject*) self;

	return buffer->size;
}

static int slice_count(Object *self)
{
	BufferSliceObject *slice = (BufferSliceObject*) self;

	return slice->length;
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

		assert(low  < 16);
		assert(high < 16);

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
	print_bytes(fp, buffer->body, buffer->size);
}

static void slice_print(Object *self, FILE *fp)
{
	BufferSliceObject *slice = (BufferSliceObject*) self;
	print_bytes(fp, slice->sliced->body + slice->offset, slice->length);
}