/*
 * 
 * -- WHAT IS THIS FILE? --------------------------------------------
 * This file implements the "static map" object. The static map 
 * object behaves like a read-only "map". (Note that "implementing 
 * an object" means a very specific thing in this interpreter. If 
 * you didn't know, check the src/objects folder.)
 * ------------------------------------------------------------------
 *
 * -- THE STATIC MAP OBJECT -----------------------------------------
 * The statis map is a read-only collection of objects. You can see 
 * it as an interface for static arrays. You can define an array of 
 * `StaticMapSlot`s and then wrap it in this object. When the map is
 * accessed, a lookup is performed into the array. Something to note
 * is that the array is converted to noja objects lazily when they 
 * are accessed, which makes the start-up times lower than a general
 * purpose map.
 * ------------------------------------------------------------------
 *
 * NOTES: 
 *   - This object, unlike the others implemented in src/objects,
 *     depends on the Runtime objects. This is because it needs
 *     to be able to create `NativeFuncObject`s. 
 *
 *   - Only strings can be keys. There is no intrinsic reason why
 *     it should be like that, it's just simpler.
 */
#include <string.h>
#include <assert.h>
#include "o_nfunc.h"
#include "o_staticmap.h"
#include "../utils/defs.h"
#include "../objects/objects.h"

typedef struct {
	Object   base;
	Runtime *runt;
	const StaticMapSlot *slots;
} StaticMapObject;

static Object *select(Object *self, Object *key, Heap *heap, Error *err);
static Object *copy(Object *self, Heap *heap, Error *err);
static int hash(Object *self);

static TypeObject t_staticmap = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "static map",
	.size = sizeof (StaticMapObject),
	.copy = copy,
	.hash = hash,
	.select = select,
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

Object *Object_NewStaticMap(const StaticMapSlot *slots, Runtime *runt, Error *error)
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

	return (Object*) obj;
}

static Object *select(Object *self, Object *key, Heap *heap, Error *error)
{
	assert(self != NULL);
	assert(self->type == &t_staticmap);
	assert(key != NULL);
	assert(heap != NULL);
	assert(error != NULL);

	StaticMapObject *map = (StaticMapObject*) self;

	if(!Object_IsString(key))
		return NULL;

	const char *name = Object_ToString(key, NULL, heap, error);

	if(map->slots == NULL)
		return NULL;

	for(int i = 0; map->slots[i].name != NULL; i += 1)
		if(!strcmp(name, map->slots[i].name))
			{
				StaticMapSlot slot = map->slots[i];
				Object *obj;
				switch(slot.kind)
					{
						case SM_BOOL:  return Object_FromBool(slot.as_bool, heap, error);
						case SM_INT:   return Object_FromInt(slot.as_int, heap, error);
						case SM_FLOAT: return Object_FromFloat(slot.as_float, heap, error);
						case SM_FUNCT: return Object_FromNativeFunction(map->runt, slot.as_funct, slot.argc, heap, error);
						case SM_STRING: return Object_FromString(slot.as_string, slot.length, heap, error);
						case SM_SMAP: return Object_NewStaticMap(slot.as_smap, map->runt, error);
						case SM_NONE: return Object_NewNone(heap, error);
						case SM_TYPE: return (Object*) slot.as_type;
						default: assert(0); break;
					}
				return obj;
			}
	return NULL;
}