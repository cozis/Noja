
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
#include "net.h"
#include "math.h"
#include "utils.h"
#include "basic.h"
#include "files.h"
#include "string.h"
#include "buffer.h"
#include "random.h"
#include "../utils/defs.h"
#include "../common/defs.h"
#include "../objects/objects.h"
#include "../runtime/runtime.h"
#include "../compiler/compile.h"

static int bin_getCurrentWorkingDirectory(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	UNUSED(argv);
	ASSERT(argc == 0);

	char path[1024];
	if(getcwd(path, sizeof(path)) == NULL) {
		Error_Report(error, 1, "Couldn't get current working directory because a buffer is too small");
		return -1;
	}

	return returnValues2(error, runtime, rets, "s", path);
}

static int bin_typename(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 1);

	Heap *heap = Runtime_GetHeap(runtime);
	const char *name = argv[0]->type->name;
	Object   *o_name = Object_FromString(name, -1, heap, error);
	if (o_name == NULL)
		return -1;

	rets[0] = o_name;
	return 1;
}

static int bin_print(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(runtime);
	UNUSED(rets);
	UNUSED(error);
	
	for(int i = 0; i < (int) argc; i += 1)
		Object_Print(argv[i], stdout);
	return 0;
}

static int bin_import(Runtime *runtime, 
					  Object **argv, 
					  unsigned int argc, 
					  Object *rets[static MAX_RETS], 
					  Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 1);
	
	ParsedArgument pargs[2];
	if (!parseArgs(error, argv, argc, pargs, "s"))
		return -1;

	const char *path = pargs[0].as_string.data;
	size_t  path_len = pargs[0].as_string.size;

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

	Source *src = Source_FromFile(full_path, error);
	if(src == NULL)
		return -1;
	
	CompilationErrorType errtyp;
    Executable *exe = compile(src, error, &errtyp);
    if(exe == NULL) {
		Source_Free(src);
		return -1;
    }

	Object *sub_rets[8];
	int retc = run(runtime, error, exe, 0, NULL, NULL, 0, sub_rets);
    if(retc < 0)
    {
		Source_Free(src);
		Executable_Free(exe);
		return -1;
    }
    ASSERT(retc == 1);
	
	Source_Free(src);
	Executable_Free(exe);
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

	size_t length;
	const char *string;

	string = Object_GetString(argv[0], &length);
	ASSERT(string != NULL);

	Error_Report(error, 0, "%s", string);
	return -1;
}

static int bin_istypeof(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	UNUSED(rets);
	UNUSED(runtime);

	ASSERT(argc == 2);
	
	Heap *heap = Runtime_GetHeap(runtime);
	bool yes = Object_IsTypeOf(argv[0], argv[1], heap, error);
	Object *o_yes = Object_FromBool(yes, heap, error);
	if (o_yes == NULL)
		return -1;

	rets[0] = o_yes;
	return 1;
}

static int bin_keysof(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	UNUSED(rets);
	UNUSED(runtime);
	ASSERT(argc == 1);
	Heap   *heap = Runtime_GetHeap(runtime);
	Object *keys = Object_KeysOf(argv[0], heap, error);
	if (keys == NULL)
		return -1;
	rets[0] = keys;
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
	slots[6].as_type = Object_GetBufferType();
	slots[7].as_type = Object_GetListType();
	slots[8].as_type = Object_GetMapType();
	slots[9].as_type = Object_GetFileType();
	slots[10].as_type = Object_GetDirType();
	slots[11].as_type = Object_GetNullableType();
	slots[12].as_type = Object_GetSumType();
	slots[13].as_object = Object_NewAny();
}

StaticMapSlot bins_basic[] = {
	{ TYPENAME_TYPE,   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_NONE,   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_INT,    SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_BOOL,   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_FLOAT,  SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_STRING, SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_BUFFER, SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_LIST,   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_MAP,    SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_FILE,   SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_DIRECTORY, SM_TYPE, .as_type = NULL /* Until bins_basic_init is called */ },
	{ TYPENAME_NULLABLE,  SM_TYPE, .as_type = NULL },
	{ TYPENAME_SUM,       SM_TYPE, .as_type = NULL },
	{ "any",    SM_OBJECT, .as_object = NULL },
	
	{ "net",    SM_SMAP, .as_smap = bins_net,    },
	{ "math",   SM_SMAP, .as_smap = bins_math,   },
	{ "files",  SM_SMAP, .as_smap = bins_files,  },
	{ "buffer", SM_SMAP, .as_smap = bins_buffer, },
	{ "string", SM_SMAP, .as_smap = bins_string, },
	{ "random", SM_SMAP, .as_smap = bins_random, },
	
	{ "import", SM_FUNCT, .as_funct = bin_import, .argc = 1, },
	{ "type",   SM_FUNCT, .as_funct = bin_type, .argc = 1 },
	{ "print",  SM_FUNCT, .as_funct = bin_print, .argc = -1 },
	{ "input",  SM_FUNCT, .as_funct = bin_input, .argc = 0 },
	{ "count",  SM_FUNCT, .as_funct = bin_count, .argc = 1 },
	{ "error",  SM_FUNCT, .as_funct = bin_error, .argc = 1 },
	{ "assert", SM_FUNCT, .as_funct = bin_assert, .argc = -1 },
	{ "istypeof", SM_FUNCT, .as_funct = bin_istypeof, .argc = 2, },
	{ "typename", SM_FUNCT, .as_funct = bin_typename, .argc = 1, },
	{ "keysof", SM_FUNCT, .as_funct = bin_keysof, .argc = 1, },
	{ "getCurrentWorkingDirectory", SM_FUNCT, .as_funct = bin_getCurrentWorkingDirectory, .argc = 0 },
	{ NULL, SM_END, {}, {} },
};
