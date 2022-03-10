#include "objects.h"

typedef struct {
	Object base;
	FILE *fp;
} FileObject;

static _Bool file_free(Object *self, Error *error);

static TypeObject t_file = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "File",
	.size = sizeof(FileObject),
	.free = file_free,
};

_Bool Object_IsFile(Object *obj)
{
	return obj->type == &t_file;
}

FILE *Object_ToStream(Object *obj, Error *error)
{
	if(!Object_IsFile(obj))
		{
			Error_Report(error, 0, "Object is not a file");
			return NULL;
		}

	return ((FileObject*) obj)->fp;
}

Object *Object_FromStream(FILE *fp, Heap *heap, Error *error)
{
	FileObject *fob = Heap_Malloc(heap, &t_file, error);

	if(fob == NULL)
		return NULL;

	fob->fp = fp;

	return (Object*) fob;
}

static _Bool file_free(Object *self, Error *error)
{	
	FileObject *fob = (FileObject*) self;
	if(fclose(fob->fp) == 0)
		return 1;

	Error_Report(error, 0, "Failed to close stream");
	return 0;
}