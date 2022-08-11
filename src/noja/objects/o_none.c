
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
#include <string.h>
#include "objects.h"

static _Bool op_eql(Object *self, Object *other);
static void   print(Object *obj, FILE *fp);
static int     hash(Object *self);
static Object *copy(Object *self, Heap *heap, Error *err);

static TypeObject t_none = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "none",
	.size = sizeof (Object),
	.hash = hash,
	.copy = copy,
	.print = print,
	.op_eql = op_eql,
};

static Object the_none_object = {
	.type = &t_none,
	.flags = Object_STATIC,
};

_Bool Object_IsNone(Object *obj)
{
	return obj == &the_none_object;
}

static int hash(Object *self)
{
	assert(self == &the_none_object);
	return 0;
}

static Object *copy(Object *self, Heap *heap, Error *err)
{
	(void) heap;
	(void) err;
	return self;
}

Object *Object_NewNone(Heap *heap, Error *error)
{
	(void) heap;
	(void) error;
	return &the_none_object;
}

static void print(Object *obj, FILE *fp)
{
	assert(fp != NULL);
	assert(obj != NULL);
	assert(obj->type == &t_none);

	fprintf(fp, "none");
}

static _Bool op_eql(Object *self, Object *other)
{
	(void) self;
	
	assert(other->type == &t_none);
	return 1;
}