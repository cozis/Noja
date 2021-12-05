#include <assert.h>
#include <string.h>
#include "../utils/defs.h"
#include "objects.h"

typedef struct {
	int           size,
	              refs;
	unsigned char data[];
} BufferBody;

typedef struct {
	Object      base;
	BufferBody *body;
	int offset, length;
} BufferObject;

static Object *select(Object *self, Object *key, Heap *heap, Error *err);
static _Bool   insert(Object *self, Object *key, Object *val, Heap *heap, Error *err);
static int     count(Object *self);
static void	   print(Object *obj, FILE *fp);

static TypeObject t_buffer = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "buffer",
	.size = sizeof (BufferObject),
	.select = select,
	.insert = insert,
	.count = count,
	.print = print,
};

Object *Object_NewBuffer(int size, Heap *heap, Error *error)
{
	assert(size >= 0);

	// Make the thing.
	BufferObject *obj;
	{
		obj = (BufferObject*) Heap_Malloc(heap, &t_buffer, error);

		if(obj == NULL)
			return NULL;

		obj->offset = 0;
		obj->length = size;
		obj->body = Heap_RawMalloc(heap, sizeof(BufferBody) + sizeof(unsigned char) * size, error);

		if(obj->body == NULL)
			return NULL;

		obj->body->size = size;
		obj->body->refs = 1;
		memset(obj->body->data, 0, size);
	}

	return (Object*) obj;
}

Object *Object_SliceBuffer(Object *buffer, int offset, int length, Heap *heap, Error *error)
{
	if(buffer->type != &t_buffer)
		{
			Error_Report(error, 0, "Not a buffer");
			return NULL;
		}

	BufferObject *original = (BufferObject*) buffer;
	BufferObject *slice = (BufferObject*) Heap_Malloc(heap, &t_buffer, error);

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

	slice->offset = original->offset + offset;
	slice->length = length;
	slice->body = original->body;
	slice->body->refs += 1;
	return (Object*) slice;
}

void *Object_GetBufferAddrAndSize(Object *obj, int *size, Error *error)
{
	if(obj->type != &t_buffer)
		{
			Error_Report(error, 0, "Not a buffer");
			return NULL;
		}

	BufferObject *buf = (BufferObject*) obj;

	if(size)
		*size = buf->length;
	return buf->body->data + buf->offset;
}

static Object *select(Object *self, Object *key, Heap *heap, Error *error)
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

	if(idx < 0 || idx >= buffer->length)
		{
			Error_Report(error, 0, "Out of range index");
			return NULL;
		}

	unsigned char byte = buffer->body->data[buffer->offset + idx];

	return Object_FromInt(byte, heap, error);
}

static _Bool insert(Object *self, Object *key, Object *val, Heap *heap, Error *error)
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

	if(idx < 0 || idx >= buffer->length)
		{
			Error_Report(error, 0, "Out of range index");
			return NULL;
		}

	unsigned char byte = Object_ToInt(val, error);

	if(error->occurred == 1)
		return 0;

	buffer->body->data[buffer->offset + idx] = byte;
	return 1;
}

static int count(Object *self)
{
	BufferObject *buffer = (BufferObject*) self;

	return buffer->length;
}

static void print(Object *self, FILE *fp)
{
	BufferObject *buffer = (BufferObject*) self;

	fprintf(fp, "[");

	for(int i = 0; i < buffer->length; i += 1)
		{
			unsigned char byte, low, high;

			byte = buffer->body->data[buffer->offset + i];
			low  = byte & 0xf;
			high = byte >> 4;

			assert(low  < 16);
			assert(high < 16);

			char c1, c2;

			c1 = low  < 10 ? low  + '0' : low  - 10 + 'A';
			c2 = high < 10 ? high + '0' : high - 10 + 'A';

			fprintf(fp, "%c%c", c1, c2);

			if(i+1 < buffer->length)
				fprintf(fp, ", ");
		}
	fprintf(fp, "]");
}