
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

#ifndef OBJECT_H
#define OBJECT_H

#include <stdio.h>
#include <dirent.h>
#include <stdbool.h>
#include "../utils/error.h"

#define MAX_RETS 8

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

struct TypeObject {

	Object base;
	
	const char *name;
	size_t      size;

	bool  	(*init)(Object *self, Error *err);
	bool  	(*free)(Object *self, Error *err);
	int 	(*hash)(Object *self);
	Object*	(*copy)(Object *self, Heap *heap, Error *err);
	int     (*call)(Object *self, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Heap *heap, Error *err);
	void 	(*print)(Object *self, FILE *fp);

	// Collections.
	Object *(*select)(Object *self, Object *key, Heap *heap, Error *err);
	Object *(*delete)(Object *self, Object *key, Heap *heap, Error *err);
	bool    (*insert)(Object *self, Object *key, Object *val, Heap *heap, Error *err);
	int 	(*count)(Object *self);

	bool (*op_eql)(Object *self, Object *other);
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
bool  	 	 Heap_StartCollection(Heap *heap, Error *error);
bool  	  	 Heap_StopCollection(Heap *heap);
void  	 	 Heap_CollectReference(Object **referer, void *heap);
float 		 Heap_GetUsagePercentage(Heap *heap);
unsigned int Heap_GetObjectCount(Heap *heap);
void        *Heap_GetPointer(Heap *heap);
unsigned int Heap_GetSize(Heap *heap);

const TypeObject* Object_GetType(const Object *obj);
const char*	      Object_GetName(const Object *obj);
int 		 Object_Hash (Object *obj);
Object*		 Object_Copy (Object *obj, Heap *heap, Error *err);
int          Object_Call (Object *obj, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Heap *heap, Error *err);
void 		 Object_Print(Object *obj, FILE *fp);
Object*		 Object_Select(Object *coll, Object *key, Heap *heap, Error *err);
Object*		 Object_Delete(Object *coll, Object *key, Heap *heap, Error *err);
bool 		 Object_Insert(Object *coll, Object *key, Object *val, Heap *heap, Error *err);
int 		 Object_Count (Object *coll, Error *err);
void 		 Object_WalkReferences(Object *parent, void (*callback)(Object **referer,                    void *userp), void *userp);
void 		 Object_WalkExtensions(Object *parent, void (*callback)(void   **referer, unsigned int size, void *userp), void *userp);

Object*		 Object_NewMap(int num, Heap *heap, Error *error);
Object*		 Object_NewList(int capacity, Heap *heap, Error *error);
Object*		 Object_NewList2(int num, Object **items, Heap *heap, Error *error);
Object*		 Object_NewNone(Heap *heap, Error *error);
Object*      Object_NewBuffer(size_t size, Heap *heap, Error *error);
Object*		 Object_NewClosure(Object *parent, Object *new_map, Heap *heap, Error *error);
Object*      Object_SliceBuffer(Object *obj, size_t offset, size_t length, Heap *heap, Error *error);

Object*		 Object_FromInt   (long long int val, Heap *heap, Error *error);
Object*		 Object_FromBool  (bool 		 val, Heap *heap, Error *error);
Object*		 Object_FromFloat (double 		 val, Heap *heap, Error *error);
Object*		 Object_FromString(const char *str, int len, Heap *heap, Error *error);
Object*		 Object_FromStream(FILE *fp, Heap *heap, Error *error);
Object* 	 Object_FromDIR(DIR *handle, Heap *heap, Error *error);

TypeObject *Object_GetTypeType();
TypeObject *Object_GetNoneType();
TypeObject *Object_GetIntType();
TypeObject *Object_GetBoolType();
TypeObject *Object_GetFloatType();
TypeObject *Object_GetStringType();
TypeObject *Object_GetListType();
TypeObject *Object_GetMapType();
TypeObject *Object_GetBufferType();
TypeObject *Object_GetFileType();
TypeObject *Object_GetDirType();

bool  Object_IsNone(Object *obj);
bool  Object_IsInt(Object *obj);
bool  Object_IsBool(Object *obj);
bool  Object_IsFloat(Object *obj);
bool  Object_IsString(Object *obj);
bool  Object_IsBuffer(Object *obj);
bool  Object_IsFile(Object *obj);
bool  Object_IsDir(Object *obj);

long long int Object_GetInt  (Object *obj);
bool  		  Object_GetBool (Object *obj);
double		  Object_GetFloat(Object *obj);
const char	 *Object_GetString(Object *obj, size_t *size);
DIR    		 *Object_GetDIR(Object *obj);
FILE   		 *Object_GetStream(Object *obj);
void         *Object_GetBuffer(Object *obj, size_t *size);

bool  		  Object_Compare(Object *obj1, Object *obj2, Error *error);


extern TypeObject t_type;
#endif