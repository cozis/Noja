#include "../common/defs.h" 
#include "objects.h"

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp);
static bool istypeof(Object *self, Object *other, Heap *heap, Error *error);

typedef struct {
    Object  base;
    Object *items[2];
} SumObject;

static TypeObject t_sum = {
    .base = (Object) { .type = &t_type, .flags = Object_STATIC },
    .name = TYPENAME_SUM,
    .size = sizeof(SumObject),
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
    .walk = walk,
    .walkexts = NULL,
};

TypeObject *Object_GetSumType()
{
    return &t_sum;
}

Object *Object_NewSum(Object *item0, Object *item1, Heap *heap, Error *error)
{
    SumObject *sum = (SumObject*) Heap_Malloc(heap, &t_sum, error);
    if (sum != NULL) {
        sum->items[0] = item0;
        sum->items[1] = item1;
    }
    return (Object*) sum;
}

static bool 
istypeof(Object *self, 
         Object *other, 
         Heap   *heap, 
         Error  *error)
{
    SumObject *sum = (SumObject*) self;
    return Object_IsTypeOf(sum->items[0], other, heap, error)
        || Object_IsTypeOf(sum->items[1], other, heap, error);
}

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp)
{
    SumObject *sum = (SumObject*) self;
    callback(sum->items + 0, userp);
    callback(sum->items + 1, userp);
}
