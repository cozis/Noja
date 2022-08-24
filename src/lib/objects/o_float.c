
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
#include "../utils/defs.h"
#include "../utils/hash.h"
#include "../common/defs.h"

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
	.name = TYPENAME_FLOAT,
	.size = sizeof (FloatObject),
	.hash = hash,
	.copy = copy,
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
	UNUSED(heap);
	UNUSED(err);
	return self; /* Float objects are immutable */
}

static _Bool op_eql(Object *self, Object *other)
{
	assert(self  != NULL && self->type  == &t_float);
	assert(other != NULL && other->type == &t_float);

	FloatObject *i1, *i2;

	i1 = (FloatObject*) self;
	i2 = (FloatObject*) other;

	return i1->val == i2->val;
}

double Object_GetFloat(Object *obj)
{
	if(!Object_IsFloat(obj)) {
		Error_Panic("%s expected a " TYPENAME_FLOAT
			        " object, but an %s was provided", 
			        __func__, Object_GetName(obj));
		return 0.0;
	}

	return ((FloatObject*) obj)->val;
}

TypeObject *Object_GetFloatType()
{
	return &t_float;
}

Object *Object_FromFloat(double val, Heap *heap, Error *error)
{
	FloatObject *obj = (FloatObject*) Heap_Malloc(heap, &t_float, error);
	if(obj == NULL)
		return NULL;

	obj->val = val;

	return (Object*) obj;
}

static void print(Object *obj, FILE *fp)
{
	assert(fp != NULL);
	assert(obj != NULL && obj->type == &t_float);

	fprintf(fp, "%2.2f", ((FloatObject*) obj)->val);
}