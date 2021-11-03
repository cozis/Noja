#ifndef OBJECT_H
#define OBJECT_H

#include <stdio.h>
#include "../utils/error.h"

typedef struct Type Type;
typedef struct Object Object;
typedef struct xHeap Heap;

struct Object {
	const Type *type;
	unsigned int flags;
};

typedef enum {
	ATMTP_NOTATOMIC = 0,
	ATMTP_INT,
	ATMTP_BOOL,
	ATMTP_FLOAT,
} AtomicType;

struct Type {

	Object base;
	
	// Any.	
	const char  *name;
	unsigned int size;
	AtomicType   atomic;

	_Bool 		 (*init)(Object *self, Error *err);
	_Bool 		 (*free)(Object *self, Error *err);
	int 		 (*hash)(Object *self);
	Object*		 (*copy)(Object *self, Heap *heap, Error *err);
	Object* 	 (*call)(Object *self, Object **argv, unsigned int argc, Heap *heap, Error *err);
	void 		 (*print)(Object *self, FILE *fp);
	unsigned int (*deepsize)(const Object *self);

	// Collections.
	Object *(*select)(Object *self, Object *key, Heap *heap, Error *err);
	Object *(*delete)(Object *self, Object *key, Heap *heap, Error *err);
	_Bool   (*insert)(Object *self, Object *key, Object *val, Heap *heap, Error *err);
	int 	(*count)(Object *self);

	// Iterators.
	Object *(*next)(Object *self, Heap *heap, Error *err);
	Object *(*prev)(Object *self, Heap *heap, Error *err);

	// Some.
	union {
		long long int (*to_int)(Object *self, Error *err);
		_Bool 		  (*to_bool)(Object *self, Error *err);
		double 		  (*to_float)(Object *self, Error *err);
		char		 *(*to_string)(Object *self, int *size, Heap *heap, Error *err);
	};

	_Bool (*op_eql)(Object *self, Object *other);
};

enum {
	Object_STATIC = 1,
};

Heap*		 Heap_New(int size);
void		 Heap_Free(Heap *heap);
void*		 Heap_Malloc   (Heap *heap, const Type *type, Error *err);
void*		 Heap_RawMalloc(Heap *heap, int         size, Error *err);

const Type*	 Object_GetType(const Object *obj);
const char*	 Object_GetName(const Object *obj);
unsigned int Object_GetSize(const Object *obj, Error *err);
unsigned int Object_GetDeepSize(const Object *obj, Error *err);
int 		 Object_Hash  (Object *obj, Error *err);
Object*		 Object_Copy  (Object *obj, Heap *heap, Error *err);
Object*		 Object_Call  (Object *obj, Object **argv, unsigned int argc, Heap *heap, Error *err);
_Bool 		 Object_Print (Object *obj, FILE *fp, Error *error);
Object*		 Object_Select(Object *coll, Object *key, Heap *heap, Error *err);
Object*		 Object_Delete(Object *coll, Object *key, Heap *heap, Error *err);
_Bool		 Object_Insert(Object *coll, Object *key, Object *val, Heap *heap, Error *err);
int 		 Object_Count (Object *coll, Error *err);
Object*		 Object_Next  (Object *iter, Heap *heap, Error *err);
Object*		 Object_Prev  (Object *iter, Heap *heap, Error *err);

Object*		 Object_NewMap(int num, Heap *heap, Error *error);
Object*		 Object_NewList(int capacity, Heap *heap, Error *error);
Object*		 Object_NewNone(Heap *heap, Error *error);

Object*		 Object_FromInt   (long long int val, Heap *heap, Error *error);
Object*		 Object_FromBool  (_Bool		 val, Heap *heap, Error *error);
Object*		 Object_FromFloat (double 		 val, Heap *heap, Error *error);
Object*		 Object_FromString(const char *str, int len, Heap *heap, Error *error);

_Bool		 Object_IsInt  (Object *obj);
_Bool		 Object_IsBool (Object *obj);
_Bool		 Object_IsFloat(Object *obj);

long long int Object_ToInt  (Object *obj, Error *err);
_Bool 		  Object_ToBool (Object *obj, Error *err);
double		  Object_ToFloat(Object *obj, Error *err);

_Bool 		  Object_Compare(Object *obj1, Object *obj2, Error *error);

extern const Type t_type;
#endif