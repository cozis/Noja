
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
#include "objects.h"
#include "../utils/defs.h"
#include "../utils/hash.h"
#include "../common/defs.h"

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
	.name = TYPENAME_INT,
	.size = sizeof(IntObject),
	.hash = hash,
	.copy = copy,
	.print = print,
	.op_eql = op_eql,
};

static int hash(Object *self)
{
	ASSERT(self != NULL && self->type == &t_int);

	IntObject *iobj = (IntObject*) self;

	return hashbytes((unsigned char*) &iobj->val, sizeof(iobj->val));
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	UNUSED(heap);
	UNUSED(err);
	return self;
}

TypeObject *Object_GetIntType()
{
	return &t_int;
}

Object *Object_FromInt(long long int val, Heap *heap, Error *error)
{
	ASSERT(heap != NULL);
	ASSERT(error != NULL);

	IntObject *obj = (IntObject*) Heap_Malloc(heap, &t_int, error);

	if(obj == 0)
		return 0;

	obj->val = val;

	return (Object*) obj;
}

long long int Object_GetInt(Object *obj)
{
	if(!Object_IsInt(obj)) {
		Error_Panic("%s expected an " TYPENAME_INT
			        " object, but an %s was provided", 
			        __func__, Object_GetName(obj));
		return 0;
	}

	return ((IntObject*) obj)->val;
}

static void print(Object *obj, FILE *fp)
{
	ASSERT(fp != NULL);
	ASSERT(obj != NULL);
	ASSERT(obj->type == &t_int);

	fprintf(fp, "%lld", ((IntObject*) obj)->val);
}

static _Bool op_eql(Object *self, Object *other)
{
	ASSERT(self != NULL);
	ASSERT(self->type == &t_int);
	ASSERT(other != NULL);
	ASSERT(other->type == &t_int);

	IntObject *i1, *i2;

	i1 = (IntObject*) self;
	i2 = (IntObject*) other;

	return i1->val == i2->val;
}
