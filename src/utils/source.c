#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "source.h"

struct xSource {
	char *name;
	char *body;
	int   size;
	int   refs;
};

Source *Source_Copy(Source *s)
{
	s->refs += 1;
	return s;
}

void Source_Free(Source *s)
{
	s->refs -= 1;
	assert(s->refs >= 0);

	if(s->refs == 0)
		free(s);
}

const char *Source_GetName(const Source *s)
{
	return s->name;
}

const char  *Source_GetBody(const Source *s)
{
	return s->body;
}

unsigned int Source_GetSize(const Source *s)
{
	return s->size;
}

Source *Source_FromFile(const char *file, Error *error)
{
	assert(file != NULL);

	// Open the file and get it's size.
	// at the end of the block, the file
	// cursor will point at the start of
	// the file.
	FILE *fp;
	int size;
	{
		fp = fopen(file, "rb");

		if(fp == NULL)
			{
				if(errno == ENOENT)
					Error_Report(error, 0, "File \"%s\" doesn't exist", file);
				else
					Error_Report(error, 1, "Call to fopen failed (%s, errno = %d)", strerror(errno), errno);
				return NULL;
			}

		if(fseek(fp, 0, SEEK_END))
			{
				Error_Report(error, 1, "Call to fseek failed (%s, errno = %d)", strerror(errno), errno);
				fclose(fp);
				return NULL;	
			}

		size = ftell(fp);

		if(size < 0)
			{
				Error_Report(error, 1, "Call to ftell failed (%s, errno = %d)", strerror(errno), errno);
				fclose(fp);
				return NULL;	
			}

		if(fseek(fp, 0, SEEK_SET))
			{
				Error_Report(error, 1, "Call to fseek failed (%s, errno = %d)", strerror(errno), errno);
				fclose(fp);
				return NULL;
			}
	}

	// Allocate the source structure.
	Source *s;
	{
		int namel = strlen(file);

		s = malloc(sizeof(Source) + namel + size + 2);

		if(s == NULL)
			{
				Error_Report(error, 1, "No memory");
				fclose(fp);
				return NULL;
			}

		s->name = (char*) (s + 1);
		s->body = s->name + namel + 1;
	}

	// Copy the name into it.
	strcpy(s->name, file);
	s->size = size;
	s->refs = 1;

	// Now copy the file contents into it.
	{
		int p = fread(s->body, 1, size, fp);

		if(p != size)
			{
				Error_Report(error, 1, "Call to fread failed, %d bytes out of %d were read (%s, errno = %d)", p, size, strerror(errno), errno);
				fclose(fp);
				free(s);
				return NULL;
			}

		s->body[s->size] = '\0';
	}

	fclose(fp);
	return s;
}

Source *Source_FromString(const char *name, const char *body, int size, Error *error)
{
	assert(body != NULL);

	if(size < 0)
		size = strlen(body);

	int namel = name ? strlen(name) : 0;

	void *memory = malloc(sizeof(Source) + namel + size + 2);

	if(memory == NULL)
		{
			Error_Report(error, 1, "No memory");
			return NULL;
		}

	Source *s = memory;
	s->name = (char*) (s + 1);
	s->body = s->name + namel + 1;
	s->size = size;
	s->refs = 1;

	if(name)
		strcpy(s->name, name);
	else
		s->name = NULL;
	
	strncpy(s->body, body, size);
	return s;
}