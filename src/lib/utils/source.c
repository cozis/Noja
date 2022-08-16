
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

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "source.h"
#include "path.h"

struct xSource {
	char *name;
	char *body;
	int   size;
	int   refs;
	bool is_file;
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

const char *Source_GetAbsolutePath(Source *src)
{
	if(src != NULL && src->is_file)
		return src->name;
	return NULL;
}

Source *Source_FromFile(const char *file, Error *error)
{
	assert(file != NULL);

	if(file[0] == '\0') {
		Error_Report(error, 0, "Empty file name");
		return NULL;
	}

	char maybe[1024];
	const char *abs_path = Path_MakeAbsolute(file, maybe, sizeof(maybe));
	if(abs_path == NULL) {
		Error_Report(error, 0, "Internal buffer is too small");
		return NULL;
	}

	// Open the file and get it's size.
	// at the end of the block, the file
	// cursor will point at the start of
	// the file.
	FILE *fp;
	int size;
	{
		fp = fopen(abs_path, "rb");

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
		int namel = strlen(abs_path);

		s = malloc(sizeof(Source) + namel + size + 2);

		if(s == NULL)
		{
			Error_Report(error, 1, "No memory");
			fclose(fp);
			return NULL;
		}

		s->is_file = true;
		s->name = (char*) (s + 1);
		s->body = s->name + namel + 1;
	}

	// Copy the name into it.
	strcpy(s->name, abs_path);
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
	s->is_file = 0;

	if(name)
		strcpy(s->name, name);
	else
		s->name = NULL;
	
	strncpy(s->body, body, size);
	return s;
}