#include "../defs.h" 
#include "../utils/defs.h"
#include "objects.h"

static bool istypeof(Object *self, Object *other, Heap *heap, Error *error);

static TypeObject t_any = {
    .base = (Object) { .type = &t_type, .flags = Object_STATIC },
    .name = TYPENAME_ANY,
    .size = sizeof(Object),
    .init = NULL,
    .free = NULL,
    .hash = NULL,
    .copy = NULL,
    .call = NULL,
    .print = NULL,

    .select = NULL,
    .delete = NULL,
    .insert = NULL,
    .count  = NULL,

    .istypeof = istypeof,

    .op_eql = NULL,
    .walk = NULL,
    .walkexts = NULL,
};

static Object the_any_object = {
    .type = &t_any,
    .flags = Object_STATIC,
};

TypeObject *Object_GetAnyType()
{
    return &t_any;
}

Object *Object_NewAny()
{
    return &the_any_object;
}

static bool 
istypeof(Object *self, 
         Object *other, 
         Heap   *heap, 
         Error  *error)
{
    UNUSED(self);
    UNUSED(other);
    UNUSED(heap);
    UNUSED(error);
    return true;
}
