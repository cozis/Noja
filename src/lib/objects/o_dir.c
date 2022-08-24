
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

#include "objects.h"
#include "../common/defs.h"

typedef struct {
	Object base;
	DIR *dir;
} DirObject;

static _Bool dir_free(Object *obj, Error *error);

static TypeObject t_dir = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = TYPENAME_DIRECTORY,
	.size = sizeof(DirObject),
	.free = dir_free,
};

TypeObject *Object_GetDirType()
{
	return &t_dir;
}

_Bool Object_IsDir(Object *obj)
{
	return Object_GetType(obj) == Object_GetDirType();
}

Object *Object_FromDIR(DIR *handle, Heap *heap, Error *error)
{
	DirObject *dob = (DirObject*) Heap_Malloc(heap, &t_dir, error);

	if(dob == NULL)
		return NULL;

	dob->dir = handle;

	return (Object*) dob;
}


DIR *Object_GetDIR(Object *obj)
{
	if(!Object_IsDir(obj)) {
		Error_Panic("%s expected a " TYPENAME_DIRECTORY
		            " object, but an %s was provided", 
		            __func__,  Object_GetName(obj));
		return NULL;
	}

	return ((DirObject*) obj)->dir;
}

static _Bool dir_free(Object *obj, Error *error)
{
	DirObject *dob = (DirObject*) obj;
	if(closedir(dob->dir) == 0)
		return 1;

	Error_Report(error, 0, "Failed to close directory");
	return 0;
}