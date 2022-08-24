
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

static void print(Object *obj, FILE *fp);
static _Bool op_eql(Object *self, Object *other);
static int hash(Object *self);
static Object *copy(Object *self, Heap *heap, Error *err);

static TypeObject t_bool = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "bool",
	.size = sizeof(Object),
	.hash = hash,
	.copy = copy,
	.print = print,
	.op_eql = op_eql,
};

static Object the_true_object = {
	.type = &t_bool,
	.flags = Object_STATIC,
};

static Object the_false_object = {
	.type = &t_bool,
	.flags = Object_STATIC,
};

static int hash(Object *self)
{
	ASSERT(self != NULL);
	ASSERT(self->type == &t_bool);

	if(self == &the_true_object)
		return 1;

	ASSERT(self == &the_false_object);
	return 0;
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	(void) heap;
	(void) err;
	return self;
}

static _Bool op_eql(Object *self, Object *other)
{
	ASSERT(self != NULL);
	ASSERT(self->type == &t_bool);
	ASSERT(other != NULL);
	ASSERT(other->type == &t_bool);

	return self == other;
}

bool Object_GetBool(Object *obj)
{
	if(!Object_IsBool(obj)) {
		Error_Panic("%s expected a bool object, but "
				    "an %s was provided", __func__, 
			        Object_GetName(obj));
		return -1;
	}

	return obj == &the_true_object;
}

TypeObject *Object_GetBoolType()
{
	return &t_bool;
}

Object *Object_FromBool(_Bool val, Heap *heap, Error *error)
{
	(void) heap;
	(void) error;
	return val ? &the_true_object : &the_false_object;
}

static void print(Object *obj, FILE *fp)
{
	ASSERT(fp != NULL);
	ASSERT(obj != NULL);
	ASSERT(obj->type == &t_bool);

	fprintf(fp, obj == &the_true_object ? "true" : "false");
}