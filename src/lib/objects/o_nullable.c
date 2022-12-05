#include "../common/defs.h" 
#include "objects.h"

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp);
static void print(Object *obj, FILE *fp);
static bool istypeof(Object *self, Object *other, Heap *heap, Error *error);

typedef struct {
    Object  base;
    Object *item;
} NullableObject;

static TypeObject t_nullable = {
    .base = (Object) { .type = &t_type, .flags = Object_STATIC },
    .name = TYPENAME_NULLABLE,
    .size = sizeof(NullableObject),
    .init = NULL,
    .free = NULL,
    .hash = NULL,
    .copy = NULL,
    .call = NULL,
    .print = print,

    .select = NULL,
    .delete = NULL,
    .insert = NULL,
    .count  = NULL,

    .istypeof = istypeof,

    .op_eql = NULL,
    .walk = walk,
    .walkexts = NULL,
};

TypeObject *Object_GetNullableType()
{
    return &t_nullable;
}

Object *Object_NewNullable(Object *item, Heap *heap, Error *error)
{
    NullableObject *nullable = (NullableObject*) Heap_Malloc(heap, &t_nullable, error);
    if (nullable != NULL)
        nullable->item = item;
    return (Object*) nullable;
}

static bool 
istypeof(Object *self, 
         Object *other, 
         Heap   *heap, 
         Error  *error)
{
    NullableObject *nullable = (NullableObject*) self;
    if (Object_IsNone(other))
        return true;
    return Object_IsTypeOf(nullable->item, other, heap, error);
}

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp)
{
    NullableObject *nullable = (NullableObject*) self;
    callback(&nullable->item, userp);
}

static void print(Object *obj, FILE *fp)
{
    NullableObject *nullable = (NullableObject*) obj;

    fprintf(fp, "?");
    Object_Print(nullable->item, fp);
}