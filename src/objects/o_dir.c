#include "objects.h"

typedef struct {
	Object base;
	DIR *dir;
} DirObject;

static _Bool dir_free(Object *obj, Error *error);

static TypeObject t_dir = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "Directory",
	.size = sizeof(DirObject),
	.free = dir_free,
};

_Bool Object_IsDir(Object *obj)
{
	return obj->type == &t_dir;
}

Object *Object_FromDIR(DIR *handle, Heap *heap, Error *error)
{
	DirObject *dob = (DirObject*) Heap_Malloc(heap, &t_dir, error);

	if(dob == NULL)
		return NULL;

	dob->dir = handle;

	return (Object*) dob;
}

DIR *Object_ToDIR(Object *obj, Error *error)
{
	if(!Object_IsDir(obj))
		{
			Error_Report(error, 0, "Object is not a directory");
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