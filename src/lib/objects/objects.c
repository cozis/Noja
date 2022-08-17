
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

static _Bool op_eql(Object *self, Object *other);

TypeObject t_type = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "type",
	.size = sizeof (TypeObject),
	.op_eql = op_eql,
};

TypeObject *Object_GetTypeType()
{
	return &t_type;
}

static _Bool op_eql(Object *self, Object *other)
{
	return self == other;
}

const char *Object_GetName(const Object *obj)
{
	ASSERT(obj != NULL);

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type);

	const char *name = type->name;
	ASSERT(name);

	return name;
}

const TypeObject *Object_GetType(const Object *obj)
{
	ASSERT(obj != NULL);
	ASSERT(obj->type != NULL);
	return obj->type;
}

unsigned int Object_GetSize(const Object *obj, Error *err)
{
	UNUSED(err);
	ASSERT(err != NULL);
	ASSERT(obj != NULL);

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type);

	return type->size;
}

unsigned int Object_GetDeepSize(const Object *obj, Error *err)
{
	ASSERT(err != NULL);
	ASSERT(obj != NULL);

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type);

	if(type->deepsize == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(obj), __func__);
		return 0;
	}

	return type->deepsize(obj);
}

int Object_Hash(Object *obj)
{
	ASSERT(obj != NULL);
	const TypeObject *type = Object_GetType(obj);
	ASSERT(type != NULL);
	ASSERT(type->hash != NULL);
	return type->hash(obj);
}

Object *Object_Copy(Object *obj, Heap *heap, Error *err)
{
	ASSERT(err != NULL);
	ASSERT(obj != NULL);

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type != NULL);

	if(type->copy == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(obj), __func__);
		return NULL;
	}

	return type->copy(obj, heap, err);
}

int Object_Call(Object *obj, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Heap *heap, Error *err)
{
	ASSERT(err != NULL && obj != NULL);

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type);

	if(type->call == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(obj), __func__);
		return -1;
	}

	return type->call(obj, argv, argc, rets, heap, err);
}

void Object_Print(Object *obj, FILE *fp)
{
	ASSERT(obj != NULL);

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type);

	if(type->print == NULL)
		fprintf(fp, "<%s is unprintable>", Object_GetName(obj));
	else
		type->print(obj, fp);
}

Object *Object_Select(Object *coll, Object *key, Heap *heap, Error *err)
{
	ASSERT(err);
	ASSERT(key);
	ASSERT(coll);

	const TypeObject *type = Object_GetType(coll);
	ASSERT(type);

	if(type->select == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(coll), __func__);
		return NULL;
	}

	return type->select(coll, key, heap, err);
}

Object *Object_Delete(Object *coll, Object *key, Heap *heap, Error *err)
{
	ASSERT(err);
	ASSERT(key);
	ASSERT(coll);

	const TypeObject *type = Object_GetType(coll);
	ASSERT(type);

	if(type->delete == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(coll), __func__);
		return NULL;
	}

	return type->delete(coll, key, heap, err);	
}

_Bool Object_Insert(Object *coll, Object *key, Object *val, Heap *heap, Error *err)
{
	ASSERT(err);
	ASSERT(key);
	ASSERT(coll);

	const TypeObject *type = Object_GetType(coll);
	ASSERT(type);

	if(type->insert == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(coll), __func__);
		return 0;
	}

	return type->insert(coll, key, val, heap, err);
}

int	Object_Count(Object *coll, Error *err)
{
	ASSERT(err);
	ASSERT(coll);

	const TypeObject *type = Object_GetType(coll);
	ASSERT(type);

	if(type->count == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(coll), __func__);
		return -1;
	}

	return type->count(coll);
}

Object *Object_Next(Object *iter, Heap *heap, Error *err)
{
	ASSERT(err);
	ASSERT(iter);

	const TypeObject *type = Object_GetType(iter);
	ASSERT(type);

	if(type->next == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(iter), __func__);
		return NULL;
	}

	return type->next(iter, heap, err);
}

Object *Object_Prev(Object *iter, Heap *heap, Error *err)
{
	ASSERT(err);
	ASSERT(iter);

	const TypeObject *type = Object_GetType(iter);
	ASSERT(type);

	if(type->prev == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(iter), __func__);
		return NULL;
	}

	return type->prev(iter, heap, err);
}

_Bool Object_IsInt(Object *obj)
{
	ASSERT(obj != NULL);
	ASSERT(obj->type != NULL);
	return obj->type->atomic == ATMTP_INT;
}

_Bool Object_IsBool(Object *obj)
{
	ASSERT(obj != NULL);
	ASSERT(obj->type != NULL);
	return obj->type->atomic == ATMTP_BOOL;
}

_Bool Object_IsFloat(Object *obj)
{
	ASSERT(obj != NULL);
	ASSERT(obj->type != NULL);
	return obj->type->atomic == ATMTP_FLOAT;
}

_Bool Object_IsString(Object *obj)
{
	ASSERT(obj != NULL);
	ASSERT(obj->type != NULL);
	return obj->type->atomic == ATMTP_STRING;
}

long long int Object_ToInt(Object *obj, Error *err)
{
	ASSERT(err);
	ASSERT(obj);

	if(!Object_IsInt(obj))
	{
		Error_Report(err, 0, "Object is not an integer");
		return 0;
	}

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type);

	if(type->to_int == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(obj), __func__);
		return 0;
	}

	return type->to_int(obj, err);
}

_Bool Object_ToBool(Object *obj, Error *err)
{
	ASSERT(err);
	ASSERT(obj);

	if(!Object_IsBool(obj))
	{
		Error_Report(err, 0, "Object is not a boolean");
		return 0;
	}

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type);

	if(type->to_bool == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(obj), __func__);
		return 0;
	}

	return type->to_bool(obj, err);
}

double Object_ToFloat(Object *obj, Error *err)
{
	ASSERT(err);
	ASSERT(obj);

	if(!Object_IsFloat(obj))
	{
		Error_Report(err, 0, "Object is not a floating");
		return 0;
	}

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type);

	if(type->to_float == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(obj), __func__);
		return 0;
	}

	return type->to_float(obj, err);
}

const char *Object_ToString(Object *obj, int *size, Heap *heap, Error *err)
{
	ASSERT(err != NULL);
	ASSERT(obj != NULL);

	if(!Object_IsString(obj))
	{
		Error_Report(err, 0, "Object is not a string");
		return NULL;
	}

	const TypeObject *type = Object_GetType(obj);
	ASSERT(type);

	if(type->to_string == NULL)
	{
		Error_Report(err, 0, "Object %s doesn't implement %s", Object_GetName(obj), __func__);
		return NULL;
	}

	return type->to_string(obj, size, heap, err);
}

_Bool Object_Compare(Object *obj1, Object *obj2, Error *error)
{
	ASSERT(obj1 != NULL);
	ASSERT(obj2 != NULL);
	ASSERT(error != NULL);

	if(obj1->type != obj2->type)
		return 0;

	if(obj1->type->op_eql == NULL)
	{
		Error_Report(error, 0, "Object %s doesn't implement %s", Object_GetName(obj1), __func__);
		return 0;
	}

	return obj1->type->op_eql(obj1, obj2);
}

void Object_WalkReferences(Object *parent, void (*callback)(Object **referer, void *userp), void *userp)
{
	ASSERT(parent != NULL);
	if(parent->type->walk != NULL)
		parent->type->walk(parent, callback, userp);
}

void Object_WalkExtensions(Object *parent, void (*callback)(void **referer, unsigned int size, void *userp), void *userp)
{
	ASSERT(parent != NULL);
	if(parent->type->walkexts != NULL)
		parent->type->walkexts(parent, callback, userp);
}