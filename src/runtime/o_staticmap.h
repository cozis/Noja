
/* Copyright (c) Francesco Cozzuto <francesco.cozzuto@gmail.com>
**
** This file is part of The Noja Interpreter.
**
** The Noja Interpreter is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at 
** your option) any later version.
**
** The Noja Interpreter is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of 
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General 
** Public License for more details.
**
** You should have received a copy of the GNU General Public License along 
** with The Noja Interpreter. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STATICMAP_H
#define STATICMAP_H

#include "../objects/objects.h"
#include "runtime.h"

typedef enum {
	SM_END,
	SM_BOOL,
	SM_INT,
	SM_FLOAT,
	SM_FUNCT,
	SM_STRING,
	SM_SMAP,
	SM_NONE,
	SM_TYPE,
} StaticMapSlotKind;

typedef struct StaticMapSlot StaticMapSlot;

struct StaticMapSlot {
	const char       *name;
	StaticMapSlotKind kind; 
	union {
		const StaticMapSlot *as_smap;
		const char          *as_string;
		_Bool                as_bool;
		long long int        as_int;
		double               as_float;
		Object            *(*as_funct)(Runtime*, Object**, unsigned int, Error*);
		TypeObject          *as_type;
	};
	union { int argc; int length; };
};

Object *Object_NewStaticMap(const StaticMapSlot *slots, Runtime *runt, Error *error);
#endif