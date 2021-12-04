#include <assert.h>
#include <string.h>
#include "../utils/defs.h"
#include "objects.h"

typedef struct {
	Object         base;
	int            size;
	unsigned char *body;
} BufferObject;

static Object *select(Object *self, Object *key, Heap *heap, Error *err);
static _Bool   insert(Object *self, Object *key, Object *val, Heap *heap, Error *err);
static int     count(Object *self);
static void	   print(Object *obj, FILE *fp);

static const Type t_buffer = {
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

		obj->size = size;
		obj->body = Heap_RawMalloc(heap, sizeof(unsigned char) * size, error);

		if(obj->body == NULL)
			return NULL;

		memset(obj->body, 0, size);
	}

	return (Object*) obj;
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

	if(idx < 0 || idx >= buffer->size)
		{
			Error_Report(error, 0, "Out of range index");
			return NULL;
		}

	unsigned char byte = buffer->body[idx];

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

	if(idx < 0 || idx >= buffer->size)
		{
			Error_Report(error, 0, "Out of range index");
			return NULL;
		}

	unsigned char byte = Object_ToInt(val, error);

	if(error->occurred == 1)
		return 0;

	buffer->body[idx] = byte;
	return 1;
}

static int count(Object *self)
{
	BufferObject *buffer = (BufferObject*) self;

	return buffer->size;
}

static void print(Object *self, FILE *fp)
{
	BufferObject *buffer = (BufferObject*) self;

	fprintf(fp, "[");

	for(int i = 0; i < buffer->size; i += 1)
		{
			unsigned char byte, low, high;

			byte = buffer->body[i];
			low  = byte & 0xf;
			high = byte >> 4;

			assert(low  < 16);
			assert(high < 16);

			char c1, c2;

			c1 = low  < 10 ? low  + '0' : low  - 10 + 'A';
			c2 = high < 10 ? high + '0' : high - 10 + 'A';

			fprintf(fp, "%c%c", c1, c2);

			if(i+1 < buffer->size)
				fprintf(fp, ", ");
		}
	fprintf(fp, "]");
}