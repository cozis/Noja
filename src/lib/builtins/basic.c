
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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "math.h"
#include "basic.h"
#include "files.h"
#include "string.h"
#include "buffer.h"
#include "../utils/defs.h"
#include "../objects/objects.h"
#include "../runtime/runtime.h"
#include "../compiler/compile.h"

static int bin_print(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(runtime);
	UNUSED(rets);
	UNUSED(error);

	for(int i = 0; i < (int) argc; i += 1)
		Object_Print(argv[i], stdout);
	return 0;
}

static int bin_import(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 1);
	
	Heap *heap = Runtime_GetHeap(runtime);
	ASSERT(heap != NULL);
	
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
		path = Object_GetString(o_path, &n);
		if (path == NULL)
			return -1;
		
		ASSERT(n >= 0);
		path_len = (size_t) n;
	}
	
	char full_path[1024];
	
	if(path[0] == '/') {
		if(path_len >= sizeof(full_path)) {
			Error_Report(error, 1, "Internal buffer is too small");
			return -1;
		}
		strcpy(full_path, path);
	} else {
		size_t written = Runtime_GetCurrentScriptFolder(runtime, full_path, sizeof(full_path));
		if(written == 0) {
			Error_Report(error, 1, "Internal buffer is too small");
			return -1;
		}

		if(written + path_len >= sizeof(full_path)) {
			Error_Report(error, 1, "Internal buffer is too small");
			return -1;
		}

		memcpy(full_path + written, path, path_len);
		full_path[written + path_len] = '\0';
	}
	
	Error sub_error;
	Error_Init(&sub_error);
	Source *src = Source_FromFile(full_path, &sub_error);
	if(src == NULL) {

		Object *o_none = Object_NewNone(heap, error);
		if(o_none == NULL)
			return -1;

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
        UNUSED(errname);
        
		Object *o_none = Object_NewNone(heap, error);
		if(o_none == NULL)
			return -1;

		Object *o_err = Object_FromString(sub_error.message, -1, heap, error);
		if(o_err == NULL)
			return -1;

		Error_Free(&sub_error);
		rets[0] = o_none;
		rets[1] = o_err;
		return 2;
    }

	Object *sub_rets[8];
	int retc = run(runtime, &sub_error, exe, 0, NULL, NULL, 0, sub_rets);
    if(retc < 0)
    {
    	const char *errname = "Runtime";
        // Snapshot?
        UNUSED(errname);

		Object *o_none = Object_NewNone(heap, error);
		if(o_none == NULL) return -1;

		Object *o_err = Object_FromString(sub_error.message, -1, heap, error);
		if(o_err == NULL) return -1;

		Error_Free(&sub_error);
		rets[0] = o_none;
		rets[1] = o_err;
		return 2;
    }
    ASSERT(retc == 1);
	
	Error_Free(&sub_error);
    rets[0] = sub_rets[0];
    return 1;
}

static int bin_type(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	ASSERT(argc == 1);
	UNUSED(runtime);
	UNUSED(error);
	UNUSED(argc);

	rets[0] = (Object*) argv[0]->type;
	return 1;
}

static int bin_count(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 1);

	int n = Object_Count(argv[0], error);

	if(error->occurred)
		return -1;

	Object *temp = Object_FromInt(n, Runtime_GetHeap(runtime), error);

	if(temp == NULL)
		return -1;

	rets[0] = temp;
	return 1;
}

static int bin_input(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argv);
	UNUSED(argc);
	ASSERT(argc == 0);

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

	rets[0] = res;
	return 1;
}

static int bin_assert(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(runtime);
	UNUSED(rets);
	
	for(unsigned int i = 0; i < argc; i += 1)
	{
		if(!Object_IsBool(argv[i]))
		{
			Error_Report(error, 0, "Argument %d isn't a boolean", i);
			return -1;
		}

		if(!Object_GetBool(argv[i]))
		{
			Error_Report(error, 0, "Assertion failed");
			return -1;
		}
	}
	return 0;
}

static int bin_error(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	UNUSED(rets);
	UNUSED(runtime);

	ASSERT(argc == 1);

	if(!Object_IsString(argv[0])) 
	{
		Error_Report(error, 0, "Argument is not a string");
		return -1;
	}

	int length;
	const char *string;

	string = Object_GetString(argv[0], &length);
	ASSERT(string != NULL);

	Error_Report(error, 0, "%s", string);
	return -1;
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
	{ "math",   SM_SMAP, .as_smap = bins_math,   },
	
	{ "files",  SM_SMAP, .as_smap = bins_files,  },
	{ "buffer", SM_SMAP, .as_smap = bins_buffer, },
	{ "string", SM_SMAP, .as_smap = bins_string, },
	
	{ "import", SM_FUNCT, .as_funct = bin_import, .argc = 1, },
	{ "type", SM_FUNCT, .as_funct = bin_type, .argc = 1 },
	{ "print", SM_FUNCT, .as_funct = bin_print, .argc = -1 },
	{ "input", SM_FUNCT, .as_funct = bin_input, .argc = 0 },
	{ "count", SM_FUNCT, .as_funct = bin_count, .argc = 1 },
	{ "error", SM_FUNCT, .as_funct = bin_error, .argc = 1 },
	{ "assert", SM_FUNCT, .as_funct = bin_assert, .argc = -1 },
	{ NULL, SM_END, {}, {} },
};
