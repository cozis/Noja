
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

#include <assert.h>
#include "objects.h"
#include "../utils/hash.h"

static double to_float(Object *obj, Error *err);
static void print(Object *obj, FILE *fp);
static _Bool op_eql(Object *self, Object *other);
static int     hash(Object *self);
static Object *copy(Object *self, Heap *heap, Error *err);

typedef struct {
	Object base;
	double val;
} FloatObject;

static TypeObject t_float = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "float",
	.size = sizeof (FloatObject),
	.atomic = ATMTP_FLOAT,
	.hash = hash,
	.copy = copy,
	.to_float = to_float,
	.print = print,
	.op_eql = op_eql
};

static int hash(Object *self)
{
	assert(self != NULL);
	assert(self->type == &t_float);

	FloatObject *iobj = (FloatObject*) self;

	return hashbytes((unsigned char*) &iobj->val, sizeof(iobj->val));
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	(void) heap;
	(void) err;
	return self;
}

static _Bool op_eql(Object *self, Object *other)
{
	assert(self != NULL);
	assert(self->type == &t_float);
	assert(other != NULL);
	assert(other->type == &t_float);

	FloatObject *i1, *i2;

	i1 = (FloatObject*) self;
	i2 = (FloatObject*) other;

	return i1->val == i2->val;
}

static double to_float(Object *obj, Error *err)
{
	assert(obj);
	assert(err);
	assert(Object_GetType(obj) == &t_float);

	(void) err;

	return ((FloatObject*) obj)->val;
}

Object *Object_FromFloat(double val, Heap *heap, Error *error)
{
	FloatObject *obj = (FloatObject*) Heap_Malloc(heap, &t_float, error);

	if(obj == 0)
		return 0;

	obj->val = val;

	return (Object*) obj;
}

static void print(Object *obj, FILE *fp)
{
	assert(fp != NULL);
	assert(obj != NULL);
	assert(obj->type == &t_float);

	fprintf(fp, "%2.2f", ((FloatObject*) obj)->val);
}