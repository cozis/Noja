
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
#include "math.h"
#include "basic.h"
#include "files.h"
#include "../utils/utf8.h"
#include "../objects/objects.h"
#include "../compiler/compile.h"
#include "../runtime/runtime.h"
static int bin_print(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	(void) runtime;
	(void) rets;
	(void) maxretc;
	(void) error;

	for(int i = 0; i < (int) argc; i += 1)
		Object_Print(argv[i], stdout);
	return 0;
}

static int bin_import(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	assert(argc == 1);
	Heap *heap = Runtime_GetHeap(runtime);
	assert(heap != NULL);

	Object *o_path = argv[0];
	const char *path;
	size_t path_len;
	{
		if(!Object_IsString(o_path))
		{
			Error_Report(error, 0, "Argument #%d is not a string", 1);
			return -1;
		}

		int n;
		path = Object_ToString(o_path, &n, Runtime_GetHeap(runtime), error);
		if (path == NULL)
			return -1;

		assert(n >= 0);
		path_len = (size_t) n;
	}

	Error sub_error;
	Error_Init(&sub_error);
	Source *src = Source_FromFile(path, &sub_error);
	if(src == NULL) {

		if(maxretc == 0)
			return 0;

		Object *o_none = Object_NewNone(heap, error);
		if(o_none == NULL)
			return -1;

		if(maxretc == 1) {
			rets[0] = o_none;
			return 1;
		}
		Object *o_err = Object_FromString(sub_error.message, -1, heap, error);
		if(o_err == NULL)
			return -1;
		Error_Free(&sub_error);

		rets[0] = o_none;
		rets[1] = o_err;
		return 2;
	}

	CompilationErrorType errtyp;
    Executable *exe = compile(src, &sub_error, &errtyp);
    if(exe == NULL) {
        const char *errname;
        switch(errtyp) {
            default:
            case CompilationErrorType_INTERNAL: errname = NULL; break;
            case CompilationErrorType_SYNTAX:   errname = "Syntax"; break;
            case CompilationErrorType_SEMANTIC: errname = "Semantic"; break;
        }
        (void) errname;
        
        {
        	if(maxretc == 0)
				return 0;

			Object *o_none = Object_NewNone(heap, error);
			if(o_none == NULL)
				return -1;

			if(maxretc == 1) {
				rets[0] = o_none;
				return 1;
			}
			Object *o_err = Object_FromString(sub_error.message, -1, heap, error);
			if(o_err == NULL)
				return -1;
			Error_Free(&sub_error);

			rets[0] = o_none;
			rets[1] = o_err;
			return 2;
        }
    }

    {
    	Object *sub_rets[8];
    	int sub_maxretc = sizeof(sub_rets)/sizeof(sub_rets[0]);
	    int retc = run(runtime, &sub_error, exe, 0, NULL, NULL, 0, sub_rets, sub_maxretc);
	    if(retc < 0)
	    {
	    	const char *errname = "Runtime";
	        // Snapshot?
	        (void) errname;
	    	{
        		if(maxretc == 0)
					return 0;

				Object *o_none = Object_NewNone(heap, error);
				if(o_none == NULL)
					return -1;

				if(maxretc == 1) {
					rets[0] = o_none;
					return 1;
				}

				Object *o_err = Object_FromString(sub_error.message, -1, heap, error);
				if(o_err == NULL)
					return -1;
				Error_Free(&sub_error);

				rets[0] = o_none;
				rets[1] = o_err;
				return 2;
	        }
	    }
	    assert(retc == 1);

	    if(maxretc == 0)
	    	return 0;
	    rets[0] = sub_rets[0];
	    return 1;
	}
	return 0;
}

static int bin_type(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	assert(argc == 1);
	(void) runtime;
	(void) error;

	if(maxretc == 0)
		return 0;
	rets[0] = (Object*) argv[0]->type;
	return 1;
}

static int bin_unicode(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	(void) runtime;
	(void) error;

	assert(argc == 1);

	uint32_t ret = 0;

	if(!Object_IsString(argv[0]))
	{
		Error_Report(error, 0, "Argument #%d is not a string", 1);
		return -1;
	}

	const char  *string;
	int n;
	string = Object_ToString(argv[0],&n,Runtime_GetHeap(runtime),error);
	if (string == NULL)
		return -1;
		
	if(n == 0)
	{
		Error_Report(error, 0, "Argument #%d is an empty string", 1);
		return -1;
	}


	int k = utf8_sequence_to_utf32_codepoint(string,n,&ret);
	assert(k >= 0);

	Object *temp = Object_FromInt(ret,Runtime_GetHeap(runtime),error);

	if(temp == NULL)
		return -1;

	if(maxretc == 0)
		return 0;
	rets[0] = temp;
	return 1;
}

static int bin_chr(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{

	assert(argc == 1);

	
	if(!Object_IsInt(argv[0]))
	{
		Error_Report(error, 0, "Argument #%d is not an integer", 1);
		return -1;
	}


	char buff[32];
	
	int value = Object_ToInt(argv[0],error);

	if(error->occurred)
		return -1;

	
	int k = utf8_sequence_from_utf32_codepoint(buff,sizeof(buff),value);

	if(k<0)
	{
		Error_Report(error, 0, "Argument #%d is not valid utf-32", 1);
		return -1;
	}
	
	Object *temp = Object_FromString(buff,k,Runtime_GetHeap(runtime),error);

	if(temp == NULL)
		return -1;

	if(maxretc == 0)
		return 0;
	rets[0] = temp;
	return 1;
}

static int bin_count(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	assert(argc == 1);

	int n = Object_Count(argv[0], error);

	if(error->occurred)
		return -1;

	Object *temp = Object_FromInt(n, Runtime_GetHeap(runtime), error);

	if(temp == NULL)
		return -1;

	if(maxretc == 0)
		return 0;
	rets[0] = temp;
	return 1;
}

static int bin_input(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
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
				return -1;
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

	if(res == NULL)
		return -1;

	if(maxretc == 0)
		return 0;
	rets[0] = res;
	return 1;
}

static int bin_assert(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	(void) runtime;
	(void) rets;
	(void) maxretc;

	for(unsigned int i = 0; i < argc; i += 1)
		if(!Object_ToBool(argv[i], error))
		{
			if(!error->occurred)
				Error_Report(error, 0, "Assertion failed");
			return -1;
		}
	return 0;
}

static int bin_error(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	(void) rets;
	(void) maxretc;

	assert(argc == 1);

	int         length;
	const char *string;

	string = Object_ToString(argv[0], &length, Runtime_GetHeap(runtime), error);

	if(string == NULL)
		return -1;

	Error_Report(error, 0, "%s", string);
	return -1;
}

static int bin_strcat(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	unsigned int total_count = 0;

	for(unsigned int i = 0; i < argc; i += 1)
	{
		if(!Object_IsString(argv[i]))
		{
			Error_Report(error, 0, "Argument #%d is not a string", i+1);
			return -1;
		}

		total_count += Object_Count(argv[i], error);

		if(error->occurred)
			return -1;
	}

	char starting[128];
	char *buffer = starting;

	if(total_count > sizeof(starting)-1)
	{
		buffer = malloc(total_count+1);

		if(buffer == NULL)
		{
			Error_Report(error, 1, "No memory");
			return -1;
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

	if(result == NULL)
		return -1;

	if(maxretc == 0)
		return 0;
	rets[0] = result;
	return 1;
}

static int bin_newBuffer(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	assert(argc == 1);

	long long int size = Object_ToInt(argv[0], error);

	if(error->occurred == 1)
		return -1;

	Object *temp = Object_NewBuffer(size, Runtime_GetHeap(runtime), error);

	if(temp == NULL)
		return -1;

	if(maxretc == 0)
		return 0;
	rets[0] = temp;
	return 1;
}

static int bin_sliceBuffer(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	assert(argc == 3);

	long long int offset = Object_ToInt(argv[1], error);
	if(error->occurred == 1) return -1;

	long long int length = Object_ToInt(argv[2], error);
	if(error->occurred == 1) return -1;

	Object *temp = Object_SliceBuffer(argv[0], offset, length, Runtime_GetHeap(runtime), error);

	if(temp == NULL)
		return -1;

	if(maxretc == 0)
		return 0;
	rets[0] = temp;
	return 1;
}

static int bin_bufferToString(Runtime *runtime, Object **argv, unsigned int argc, Object **rets, unsigned int maxretc, Error *error)
{
	assert(argc == 1);

	void *buffaddr;
	int   buffsize;

	buffaddr = Object_GetBufferAddrAndSize(argv[0], &buffsize, error);

	if(error->occurred)
		return -1;

	Object *temp =  Object_FromString(buffaddr, buffsize, Runtime_GetHeap(runtime), error);

	if(temp == NULL)
		return -1;

	if(maxretc == 0)
		return 0;
	rets[0] = temp;
	return 1;
}

void bins_basic_init(StaticMapSlot slots[])
{
	slots[0].as_type = Object_GetTypeType();
	slots[1].as_type = Object_GetNoneType();
	slots[2].as_type = Object_GetIntType();
	slots[3].as_type = Object_GetBoolType();
	slots[4].as_type = Object_GetFloatType();
	slots[5].as_type = Object_GetStringType();
	slots[6].as_type = Object_GetListType();
	slots[7].as_type = Object_GetMapType();
	slots[8].as_type = Object_GetFileType();
	slots[9].as_type = Object_GetDirType();
}

StaticMapSlot bins_basic[] = {

	{ "Type",   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ "None",   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ "int",    SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ "bool",   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ "float",  SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ "String", SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ "List",   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ "Map",    SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ "File",   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ "Dir",    SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },

	{ "math",  SM_SMAP, .as_smap = bins_math, },
//	{ "files", SM_SMAP, .as_smap = bins_files, },
//	{ "net",  SM_SMAP, .as_smap = bins_net, },

	{ "import", SM_FUNCT, .as_funct = bin_import, .argc = 1, },
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
