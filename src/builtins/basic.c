
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "basic.h"
#include "files.h"
#include "math.h"
#include "../utils/utf8.h"

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


static Object *bin_unicode(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	(void) runtime;
	(void) error;

	assert(argc == 1);

	uint32_t ret = 0;

	if(!Object_IsString(argv[0]))
	{
		Error_Report(error, 0, "Argument #%d is not a string", 1);
		return NULL;
	}

	const char  *string;
	int n;
	string = Object_ToString(argv[0],&n,Runtime_GetHeap(runtime),error);
	if (string == NULL)
		return NULL;
		
	if(n == 0)
	{
		Error_Report(error, 0, "Argument #%d is an empty string", 1);
		return NULL;
	}


	int k = utf8_sequence_to_utf32_codepoint(string,n,&ret);
	assert(k >= 0);
	
	return Object_FromInt(ret,Runtime_GetHeap(runtime),error);
}

static Object *bin_chr(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{

	assert(argc == 1);

	
	if(!Object_IsInt(argv[0]))
		{
			Error_Report(error, 0, "Argument #%d is not a integer", 1);
			return NULL;
		}


	char buff[32];
	
	int value = Object_ToInt(argv[0],error);

	if(error->occurred)
		return NULL;

	
	int k = utf8_sequence_from_utf32_codepoint(buff,sizeof(buff),value);

	assert(k >= 0);
	
	return Object_FromString(buff,k,Runtime_GetHeap(runtime),error);
}

static Object *bin_count(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	int n = Object_Count(argv[0], error);

	if(error->occurred)
		return NULL;

	return Object_FromInt(n, Runtime_GetHeap(runtime), error);
}

static Object *bin_input(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	(void) argv;
	
	assert(argc == 0);

	char maybe[256];
	char *str = maybe;
	int size = 0, cap = sizeof(maybe)-1;

	while(1)
	{
		char c = getc(stdin);

		if(c == '\n')
			break;

		if(size == cap)
		{
			int newcap = cap*2;
			char *tmp = realloc(str, newcap + 1);

			if(tmp == NULL)
			{
				if(str != maybe) free(str);
				Error_Report(error, 1, "No memory");
				return NULL;
			}

			str = tmp;
			cap = newcap;
		}

		str[size++] = c;
	}

	str[size] = '\0';

	Object *res = Object_FromString(str, size, Runtime_GetHeap(runtime), error);

	if(str != maybe) 
		free(str);

	return res;
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

static Object *bin_error(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	int         length;
	const char *string;

	string = Object_ToString(argv[0], &length, Runtime_GetHeap(runtime), error);

	if(string == NULL)
		return NULL;

	Error_Report(error, 0, "%s", string);
	return NULL;
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
		int n;
		const char *s = Object_ToString(argv[i], &n, 
			Runtime_GetHeap(runtime), error);

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
	{ "unicode", SM_FUNCT, .as_funct = bin_unicode, .argc = 1 },
	{ "chr", SM_FUNCT, .as_funct = bin_chr, .argc = 1 },
	{ "print", SM_FUNCT, .as_funct = bin_print, .argc = -1 },
	{ "input", SM_FUNCT, .as_funct = bin_input, .argc = 0 },
	{ "count", SM_FUNCT, .as_funct = bin_count, .argc = 1 },
	{ "error", SM_FUNCT, .as_funct = bin_error, .argc = 1 },
	{ "assert", SM_FUNCT, .as_funct = bin_assert, .argc = -1 },
	{ NULL, SM_END, {}, {} },
};
