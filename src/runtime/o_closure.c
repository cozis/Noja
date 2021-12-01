// NOTE: This data structure doesn't strictly depend on
//       the runtime, so it could be moved to src/objects.

#include "../utils/defs.h"
#include "../objects/objects.h"

typedef struct ClosureObject ClosureObject;

struct ClosureObject {
	Object         base;
	ClosureObject *prev;
	Object        *vars;
};

static Object *select(Object *self, Object *key, Heap *heap, Error *err);

static const Type t_closure = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "closure",
	.size = sizeof(ClosureObject),
	.select = select,
};

Object *Object_NewClosure(Object *parent, Object *new_map, Heap *heap, Error *error)
{
	ClosureObject *obj = (ClosureObject*) Heap_Malloc(heap, &t_closure, error);

	if(obj == NULL)
		return NULL;

	if(parent != NULL && parent->type != &t_closure)
		{
			Error_Report(error, 0, "Object is not a closure");
			return NULL;
		}

	obj->prev = (ClosureObject*) parent;
	obj->vars = new_map;

	return (Object*) obj;
}

static Object *select(Object *self, Object *key, Heap *heap, Error *err)
{
	ClosureObject *closure = (ClosureObject*) self;

	Object *selected = NULL;

	while(closure != NULL && selected == NULL)
		{
			selected = Object_Select(closure->vars, key, heap, err);

			if(err->occurred)
				return NULL;

			closure = closure->prev;
		}

	return selected;
}