
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
#include "objects.h"
#include "../utils/hash.h"

static long long int to_int(Object *obj, Error *err);
static void print(Object *obj, FILE *fp);
static _Bool op_eql(Object *self, Object *other);
static int hash(Object *self);
static Object *copy(Object *self, Heap *heap, Error *err);

typedef struct {
	Object base;
	long long int val;
} IntObject;

static TypeObject t_int = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "int",
	.size = sizeof (IntObject),
	.atomic = ATMTP_INT,
	.hash = hash,
	.copy = copy,
	.to_int = to_int,
	.print = print,
	.op_eql = op_eql,
};

static int hash(Object *self)
{
	assert(self != NULL);
	assert(self->type == &t_int);

	IntObject *iobj = (IntObject*) self;

	return hashbytes((unsigned char*) &iobj->val, sizeof(iobj->val));
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	(void) heap;
	(void) err;
	return self;
}

static long long int to_int(Object *obj, Error *err)
{
	assert(obj != NULL);
	assert(err != NULL);
	assert(Object_GetType(obj) == &t_int);

	(void) err;

	return ((IntObject*) obj)->val;
}

Object *Object_FromInt(long long int val, Heap *heap, Error *error)
{
	assert(heap != NULL);
	assert(error != NULL);

	IntObject *obj = (IntObject*) Heap_Malloc(heap, &t_int, error);

	if(obj == 0)
		return 0;

	obj->val = val;

	return (Object*) obj;
}

static void print(Object *obj, FILE *fp)
{
	assert(fp != NULL);
	assert(obj != NULL);
	assert(obj->type == &t_int);

	fprintf(fp, "%lld", ((IntObject*) obj)->val);
}

static _Bool op_eql(Object *self, Object *other)
{
	assert(self != NULL);
	assert(self->type == &t_int);
	assert(other != NULL);
	assert(other->type == &t_int);

	IntObject *i1, *i2;

	i1 = (IntObject*) self;
	i2 = (IntObject*) other;

	return i1->val == i2->val;
}