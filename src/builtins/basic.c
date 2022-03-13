
/* Copyright (c) 2022 Francesco Cozzuto <francesco.cozzuto@gmail.com>
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "basic.h"
#include "files.h"
#include "math.h"

static Object *bin_print(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	for(int i = 0; i < (int) argc; i += 1)
		Object_Print(argv[i], stdout);

	return Object_NewNone(Runtime_GetHeap(runtime), error);
}

static Object *bin_type(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);
	(void) runtime;
	(void) error;
	return (Object*) argv[0]->type;
}

static Object *bin_count(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	int n = Object_Count(argv[0], error);

	if(error->occurred)
		return NULL;

	return Object_FromInt(n, Runtime_GetHeap(runtime), error);
}

static Object *bin_assert(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	for(unsigned int i = 0; i < argc; i += 1)
		if(!Object_ToBool(argv[i], error))
			{
				if(!error->occurred)
					Error_Report(error, 0, "Assertion failed");
				return NULL;
			}
	return Object_NewNone(Runtime_GetHeap(runtime), error);
}

static Object *bin_strcat(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	unsigned int total_count = 0;

	for(unsigned int i = 0; i < argc; i += 1)
		{
			if(!Object_IsString(argv[i]))
				{
					Error_Report(error, 0, "Argument #%d is not a string", i+1);
					return NULL;
				}

			total_count += Object_Count(argv[i], error);

			if(error->occurred)
				return NULL;
		}

	char starting[128];
	char *buffer = starting;

	if(total_count > sizeof(starting)-1)
		{
			buffer = malloc(total_count+1);

			if(buffer == NULL)
				{
					Error_Report(error, 1, "No memory");
					return NULL;
				}	
		}

	Object *result = NULL;

	for(unsigned int i = 0, written = 0; i < argc; i += 1)
		{
			int         n;
			const char *s;

			s = Object_ToString(argv[i], &n, Runtime_GetHeap(runtime), error);

			if(error->occurred)
				goto done;

			memcpy(buffer + written, s, n);
			written += n;
		}

	buffer[total_count] = '\0';

	result = Object_FromString(buffer, total_count, Runtime_GetHeap(runtime), error);

done:
	if(starting != buffer)
		free(buffer);
	return result;
}

static Object *bin_newBuffer(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	long long int size = Object_ToInt(argv[0], error);

	if(error->occurred == 1)
		return NULL;

	return Object_NewBuffer(size, Runtime_GetHeap(runtime), error);
}

static Object *bin_sliceBuffer(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 3);

	long long int offset = Object_ToInt(argv[1], error);
	if(error->occurred == 1) return NULL;

	long long int length = Object_ToInt(argv[2], error);
	if(error->occurred == 1) return NULL;

	return Object_SliceBuffer(argv[0], offset, length, Runtime_GetHeap(runtime), error);
}

static Object *bin_bufferToString(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	void *buffaddr;
	int   buffsize;

	buffaddr = Object_GetBufferAddrAndSize(argv[0], &buffsize, error);

	if(error->occurred)
		return NULL;

	return Object_FromString(buffaddr, buffsize, Runtime_GetHeap(runtime), error);
}

const StaticMapSlot bins_basic[] = {
	{ "math",  SM_SMAP, .as_smap = bins_math, },
	{ "files", SM_SMAP, .as_smap = bins_files, },
//	{ "net",  SM_SMAP, .as_smap = bins_net, },

	{ "newBuffer",   SM_FUNCT, .as_funct = bin_newBuffer, .argc = 1 },
	{ "sliceBuffer", SM_FUNCT, .as_funct = bin_sliceBuffer, .argc = 3 },
	{ "bufferToString", SM_FUNCT, .as_funct = bin_bufferToString, .argc = 1 },

	{ "strcat", SM_FUNCT, .as_funct = bin_strcat, .argc = -1 },

	{ "type", SM_FUNCT, .as_funct = bin_type, .argc = 1 },
	{ "print", SM_FUNCT, .as_funct = bin_print, .argc = -1 },
	{ "count", SM_FUNCT, .as_funct = bin_count, .argc = 1 },
	{ "assert", SM_FUNCT, .as_funct = bin_assert, .argc = -1 },
	{ NULL, SM_END, {}, {} },
};