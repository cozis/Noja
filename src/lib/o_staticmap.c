
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
** |                         WHAT IS THIS FILE?                               |
** | This file implements the "static map" object. The static map object      |
** | behaves like a read-only "map". (Note that "implementing an object" means|
** | a very specific thing in this interpreter. If you didn't know, check the |
** | src/objects folder.)                                                     |
** |                                                                          |
** |                        THE STATIC MAP OBJECT                             |
** | The statis map is a read-only collection of objects. You can see it as   |
** | an interface for static arrays. You can define an array of               |
** | `StaticMapSlot`s and then wrap it in this object. When the map is        |
** | accessed, a lookup is performed into the array. Something to note is that| 
** | the array is converted to noja objects lazily when they are accessed,    |
** | which makes the start-up times lower than a general purpose map.         |
** +--------------------------------------------------------------------------+
** | NOTES:                                                                   |
** |  - Only strings can be keys. There is no intrinsic reason why            |
** |    it should be like that, it's just simpler.                            |
** +--------------------------------------------------------------------------+
*/
#include <string.h>
#include <assert.h>
#include "runtime.h"
#include "utils/defs.h"
#include "objects/objects.h"

typedef struct {
	Object   base;
	Runtime *runt;
	const StaticMapSlot *slots;
} StaticMapObject;

static Object *select_(Object *self, Object *key, Heap *heap, Error *err);
static Object *copy(Object *self, Heap *heap, Error *err);
static int hash(Object *self);

static TypeObject t_staticmap = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "static map",
	.size = sizeof (StaticMapObject),
	.copy = copy,
	.hash = hash,
	.select = select_,
#warning "Need to walk the references"
};

static Object *copy(Object *self, Heap *heap, Error *err)
{
	(void) heap;
	(void) err;
	return self;
}

static int hash(Object *self)
{
	(void) self;
	return 0;
}

Object *Object_NewStaticMap(StaticMapSlot slots[], void (*initfn)(StaticMapSlot[]), Runtime *runt, Error *error)
{
	Heap *heap = Runtime_GetHeap(runt);

	// Make the thing.
	StaticMapObject *obj = (StaticMapObject*) Heap_Malloc(heap, &t_staticmap, error);
	{
		if(obj == 0)
			return 0;

		obj->runt = runt;
		obj->slots = slots;
	}

	if(initfn != NULL)
		initfn(slots);

	return (Object*) obj;
}

static Object *select_(Object *self, Object *key, Heap *heap, Error *error)
{
	assert(self != NULL);
	assert(self->type == &t_staticmap);
	assert(key != NULL);
	assert(heap != NULL);
	assert(error != NULL);

	StaticMapObject *map = (StaticMapObject*) self;

	if(!Object_IsString(key))
		return NULL;

	const char *name = Object_GetString(key, NULL);
	assert(name != NULL);

	if(map->slots == NULL)
		return NULL;

	for(int i = 0; map->slots[i].name != NULL; i += 1)
		if(!strcmp(name, map->slots[i].name))
		{
			StaticMapSlot slot = map->slots[i];
			Object *obj = NULL;
			switch(slot.kind)
			{
				case SM_BOOL:  return Object_FromBool(slot.as_bool, heap, error);
				case SM_INT:   return Object_FromInt(slot.as_int, heap, error);
				case SM_FLOAT: return Object_FromFloat(slot.as_float, heap, error);
				case SM_FUNCT: return Object_FromNativeFunction(map->runt, slot.as_funct, slot.argc, heap, error);
				case SM_STRING: return Object_FromString(slot.as_string, slot.length, heap, error);
				case SM_SMAP: return Object_NewStaticMap(slot.as_smap, NULL, map->runt, error);
				case SM_NONE: return Object_NewNone(heap, error);
				case SM_TYPE: return (Object*) slot.as_type;
				case SM_OBJECT: return slot.as_object;
				default: assert(0); break;
			}
			return obj;
		}
	return NULL;
}