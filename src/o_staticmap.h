#ifndef STATICMAP_H
#define STATICMAP_H

#include "objects/objects.h"
#include "runtime/runtime.h"

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