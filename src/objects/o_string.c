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
static Object *copy(Object *self, Heap *heap, Error *err);
static void print(Object *obj, FILE *fp);
static char *to_string(Object *self, int *size, Heap *heap, Error *err);
static _Bool op_eql(Object *self, Object *other);
static void walkexts(Object *self, void (*callback)(void **referer, unsigned int size, void *userp), void *userp);

static TypeObject t_string = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "string",
	.atomic = ATMTP_STRING,
	.size = sizeof(StringObject),
	.hash = hash,
	.count = count,
	.copy = copy,
	.print = print,
	.to_string = to_string,
	.op_eql = op_eql,
	.walkexts = walkexts,
};

static char *to_string(Object *self, int *size, Heap *heap, Error *err)
{
	assert(self != NULL);
	assert(self->type == &t_string);

	(void) heap;
	(void) err;

	StringObject *s = (StringObject*) self;

	if(size)
		*size = s->size;

	return s->body;
}

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

static Object *copy(Object *self, Heap *heap, Error *err)
{
	assert(self != NULL);
	assert(self->type == &t_string);
	assert(heap != NULL);
	assert(err != NULL);

	return self;
}

static _Bool op_eql(Object *self, Object *other)
{
	assert(self != NULL);
	assert(self->type == &t_string);
	assert(other != NULL);
	assert(other->type == &t_string);

	StringObject *s1 = (StringObject*) self;
	StringObject *s2 = (StringObject*) other;

	_Bool match = s1->size == s2->size && !strncmp(s1->body, s2->body, s1->size);

	return match;
}

static void print(Object *obj, FILE *fp)
{
	assert(fp != NULL);
	assert(obj != NULL);
	assert(obj->type == &t_string);

	StringObject *str = (StringObject*) obj;

	fprintf(fp, "%.*s", str->size, str->body);
}

static void walkexts(Object *self, void (*callback)(void **referer, unsigned int size, void *userp), void *userp)
{
	StringObject *str = (StringObject*) self;
	
	callback((void**) &str->body, str->size+1, userp);
}