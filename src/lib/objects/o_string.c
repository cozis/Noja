
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

#include <string.h>
#include "../utils/defs.h"
#include "../utils/hash.h"
#include "../utils/utf8.h"
#include "objects.h"

typedef struct {
	Object  base;
	int     count;
	int     bytes;
	char   *body;
} StringObject;

static int hash(Object *self);
static int count(Object *self);
static Object *copy(Object *self, Heap *heap, Error *err);
static void print(Object *obj, FILE *fp);
static char *to_string(Object *self, int *size, Heap *heap, Error *err);
static _Bool op_eql(Object *self, Object *other);
static void walkexts(Object *self, void (*callback)(void **referer, unsigned int size, void *userp), void *userp);
static Object *select(Object *self, Object *key, Heap *heap, Error *error);

static TypeObject t_string = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "string",
	.atomic = ATMTP_STRING,
	.size = sizeof(StringObject),
	.hash = hash,
	.count = count,
	.copy = copy,
	.print = print,
	.select = select,
	.to_string = to_string,
	.op_eql = op_eql,
	.walkexts = walkexts,
};

static int char_index_to_offset(StringObject *str, int idx)
{
	if(str->count == str->bytes)
		return idx;

	// Iterate over a string to find the first byte of
	// the utf-8 character number [idx].

	int scanned_bytes = 0, 
	    last_code_len = 0;

	while(idx > 0)
	{
		last_code_len = utf8_sequence_to_utf32_codepoint(str->body + scanned_bytes, str->bytes - scanned_bytes, NULL);
		scanned_bytes += last_code_len;
		idx -= 1;

		ASSERT(scanned_bytes <= str->bytes);
	}

	ASSERT(idx == 0);
	return scanned_bytes;
}

static Object *select(Object *self, Object *key, Heap *heap, Error *error)
{
	ASSERT(self != NULL && self->type == &t_string);
	ASSERT(key != NULL && heap != NULL && error != NULL);

	if(!Object_IsInt(key))
	{
		Error_Report(error, 0, "Non integer key");
		return NULL;
	}

	int idx = Object_ToInt(key, error);
	ASSERT(error->occurred == 0);

	StringObject *str = (StringObject*) self;

	if(idx < 0 || idx >= str->count)
	{
		Error_Report(error, 0, "Out of range index");
		return NULL;
	}

	int byteoffset = char_index_to_offset(str, idx);
	int codelength = utf8_sequence_to_utf32_codepoint(str->body + byteoffset, str->bytes - byteoffset, NULL);

	return Object_FromString(str->body + byteoffset, codelength, heap, error);
}

static char *to_string(Object *self, int *size, Heap *heap, Error *err)
{
	ASSERT(self != NULL);
	ASSERT(self->type == &t_string);

	(void) heap;
	(void) err;

	StringObject *s = (StringObject*) self;

	if(size)
		*size = s->bytes;

	return s->body;
}

TypeObject *Object_GetStringType()
{
	return &t_string;
}

Object *Object_FromString(const char *str, int len, Heap *heap, Error *error)
{
	ASSERT(str != NULL);
	ASSERT(heap != NULL);
	ASSERT(error != NULL);

	if(len < 0)
		len = strlen(str);

	int count = utf8_strlen(str, len);

	if(count < 0)
	{
		Error_Report(error, 0, "Invalid UTF-8 sequence");
		return NULL;
	}

	StringObject *strobj = Heap_Malloc(heap, &t_string, error);

	if(strobj == NULL)
		return NULL;

	strobj->body = Heap_RawMalloc(heap, len+1, error);
	strobj->bytes = len;
	strobj->count = count;

	if(strobj->body == NULL)
		return NULL;

	memcpy(strobj->body, str, len);

	strobj->body[len] = '\0';

	return (Object*) strobj;
}

static int count(Object *self)
{
	ASSERT(self != NULL);
	ASSERT(self->type == &t_string);

	StringObject *strobj = (StringObject*) self;

	return strobj->count;
}

static int hash(Object *self)
{
	ASSERT(self != NULL);
	ASSERT(self->type == &t_string);

	StringObject *strobj = (StringObject*) self;

	return hashbytes((unsigned char*) strobj->body, strobj->count);
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	UNUSED(heap);
	UNUSED(err);
	ASSERT(self != NULL);
	ASSERT(self->type == &t_string);
	ASSERT(heap != NULL);
	ASSERT(err != NULL);

	return self;
}

static _Bool op_eql(Object *self, Object *other)
{
	ASSERT(self != NULL);
	ASSERT(self->type == &t_string);
	ASSERT(other != NULL);
	ASSERT(other->type == &t_string);

	StringObject *s1 = (StringObject*) self;
	StringObject *s2 = (StringObject*) other;

	_Bool match = s1->bytes == s2->bytes && !strncmp(s1->body, s2->body, s1->bytes);

	return match;
}

static void print(Object *obj, FILE *fp)
{
	ASSERT(fp != NULL);
	ASSERT(obj != NULL);
	ASSERT(obj->type == &t_string);

	StringObject *str = (StringObject*) obj;

	fprintf(fp, "%.*s", str->bytes, str->body);
}

static void walkexts(Object *self, void (*callback)(void **referer, unsigned int size, void *userp), void *userp)
{
	StringObject *str = (StringObject*) self;
	
	callback((void**) &str->body, str->bytes+1, userp);
}