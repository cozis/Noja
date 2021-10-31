#include <string.h>
#include <assert.h>
#include "../utils/defs.h"
#include "../utils/hash.h"
#include "objects.h"

typedef struct {
	Object  base;
	int     size;
	char   *body;
} StringObject;

static int hash(Object *self);
static int count(Object *self);

static const Type t_string = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "string",
	.size = sizeof (StringObject),
	.hash = hash,
	.count = count,
};

Object *Object_FromString(const char *str, int len, Heap *heap, Error *error)
{
	assert(str != NULL);
	assert(heap != NULL);
	assert(error != NULL);

	if(len < 0)
		len = strlen(str);

	StringObject *strobj = Heap_Malloc(heap, &t_string, error);

	if(strobj == NULL)
		return NULL;

	strobj->body = Heap_RawMalloc(heap, len+1, error);
	strobj->size = len;

	if(strobj->body == NULL)
		return NULL;

	memcpy(strobj->body, str, len);

	strobj->body[len] = '\0';

	return (Object*) strobj;
}

static int count(Object *self)
{
	assert(self != NULL);
	assert(self->type == &t_string);

	StringObject *strobj = (StringObject*) self;

	return strobj->size;
}

static int hash(Object *self)
{
	assert(self != NULL);
	assert(self->type == &t_string);

	StringObject *strobj = (StringObject*) self;

	return hashbytes((unsigned char*) strobj->body, strobj->size);
}