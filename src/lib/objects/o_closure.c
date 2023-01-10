
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

#include "../utils/defs.h"
#include "objects.h"

typedef struct ClosureObject ClosureObject;

struct ClosureObject {
	Object  base;
	Object *prev;
	Object *vars;
};

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp);
static Object *select_(Object *self, Object *key, Heap *heap, Error *err);

static TypeObject t_closure = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "Closure",
	.size = sizeof(ClosureObject),
	.select = select_,
	.walk = walk,
};

Object *Object_NewClosure(Object *parent, Object *new_map, Heap *heap, Error *error)
{
	ClosureObject *obj = (ClosureObject*) Heap_Malloc(heap, &t_closure, error);

	if(obj == NULL)
		return NULL;

	obj->prev = parent;
	obj->vars = new_map;

	return (Object*) obj;
}

static Object *select_(Object *self, Object *key, 
	                   Heap *heap, Error *err)
{
	ClosureObject *closure = (ClosureObject*) self;

	Object *selected = Object_Select(closure->vars, key, heap, err);
	if(err->occurred)
		return NULL;

	if (selected == NULL && closure->prev != NULL)
		selected = Object_Select(closure->prev, key, heap, err);

	return selected;
}

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp)
{
	ClosureObject *closure = (ClosureObject*) self;

	callback(&closure->prev, userp);
	callback(&closure->vars, userp);
}