#ifndef OBJECT_H
#define OBJECT_H

#include <dirent.h>
#include <stdio.h>
#include "../utils/error.h"

typedef struct TypeObject TypeObject;
typedef struct Object Object;
typedef struct xHeap Heap;

struct Object {
	TypeObject *type;
	unsigned int flags;
};

typedef struct {
	Object base;
	Object *new_location;
} MovedObject;

typedef enum {
	ATMTP_NOTATOMIC = 0,
	ATMTP_INT,
	ATMTP_BOOL,
	ATMTP_FLOAT,
	ATMTP_STRING,
} AtomicType;

struct TypeObject {

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

	// All.
	void (*walk)    (Object *self, void (*callback)(Object **referer,                    void *userp), void *userp);
	void (*walkexts)(Object *self, void (*callback)(void   **referer, unsigned int size, void *userp), void *userp);
};

enum {
	Object_STATIC = 1,
	Object_MOVED  = 2,
};

Heap*		 Heap_New(int size);
void		 Heap_Free(Heap *heap);
void*		 Heap_Malloc   (Heap *heap, TypeObject *type, Error *err);
void*		 Heap_RawMalloc(Heap *heap, int size, Error *err);
_Bool 	 	 Heap_StartCollection(Heap *heap, Error *error);
_Bool 	  	 Heap_StopCollection(Heap *heap);
void  	 	 Heap_CollectReference(Object **referer, void *heap);
float 		 Heap_GetUsagePercentage(Heap *heap);
unsigned int Heap_GetObjectCount(Heap *heap);
void        *Heap_GetPointer(Heap *heap);
unsigned int Heap_GetSize(Heap *heap);

const TypeObject* Object_GetType(const Object *obj);
const char*	 Object_GetName(const Object *obj);
unsigned int Object_GetSize(const Object *obj, Error *err);
unsigned int Object_GetDeepSize(const Object *obj, Error *err);
void        *Object_GetBufferAddrAndSize(Object *obj, int *size, Error *error);
int 		 Object_Hash  (Object *obj);
Object*		 Object_Copy  (Object *obj, Heap *heap, Error *err);
Object*		 Object_Call  (Object *obj, Object **argv, unsigned int argc, Heap *heap, Error *err);
void 		 Object_Print (Object *obj, FILE *fp);
Object*		 Object_Select(Object *coll, Object *key, Heap *heap, Error *err);
Object*		 Object_Delete(Object *coll, Object *key, Heap *heap, Error *err);
_Bool		 Object_Insert(Object *coll, Object *key, Object *val, Heap *heap, Error *err);
int 		 Object_Count (Object *coll, Error *err);
Object*		 Object_Next  (Object *iter, Heap *heap, Error *err);
Object*		 Object_Prev  (Object *iter, Heap *heap, Error *err);
void 		 Object_WalkReferences(Object *parent, void (*callback)(Object **referer,                    void *userp), void *userp);
void 		 Object_WalkExtensions(Object *parent, void (*callback)(void   **referer, unsigned int size, void *userp), void *userp);

Object*		 Object_NewMap(int num, Heap *heap, Error *error);
Object*		 Object_NewList(int capacity, Heap *heap, Error *error);
Object*		 Object_NewList2(int num, Object **items, Heap *heap, Error *error);
Object*		 Object_NewNone(Heap *heap, Error *error);
Object*		 Object_NewBuffer(int size, Heap *heap, Error *error);
Object*		 Object_NewClosure(Object *parent, Object *new_map, Heap *heap, Error *error);
Object*		 Object_SliceBuffer(Object *buffer, int offset, int length, Heap *heap, Error *error);

Object*		 Object_FromInt   (long long int val, Heap *heap, Error *error);
Object*		 Object_FromBool  (_Bool		 val, Heap *heap, Error *error);
Object*		 Object_FromFloat (double 		 val, Heap *heap, Error *error);
Object*		 Object_FromString(const char *str, int len, Heap *heap, Error *error);
Object*		 Object_FromStream(FILE *fp, Heap *heap, Error *error);
Object* 	 Object_FromDIR(DIR *handle, Heap *heap, Error *error);

_Bool Object_IsNone(Object *obj);
_Bool Object_IsInt(Object *obj);
_Bool Object_IsBool(Object *obj);
_Bool Object_IsFloat(Object *obj);
_Bool Object_IsString(Object *obj);
_Bool Object_IsBuffer(Object *obj);
_Bool Object_IsFile(Object *obj);
_Bool Object_IsDir(Object *obj);

long long int Object_ToInt  (Object *obj, Error *err);
_Bool 		  Object_ToBool (Object *obj, Error *err);
double		  Object_ToFloat(Object *obj, Error *err);
const char	 *Object_ToString(Object *obj, int *size, Heap *heap, Error *err);
DIR    		 *Object_ToDIR(Object *obj, Error *error);
FILE   		 *Object_ToStream(Object *obj, Error *error);

_Bool 		  Object_Compare(Object *obj1, Object *obj2, Error *error);


extern TypeObject t_type;
#endif