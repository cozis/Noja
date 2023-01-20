
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

#include <errno.h>
#include "files.h"
#include "utils.h"
#include "../utils/defs.h"

enum {
	MD_READ = 0,
	MD_WRITE = 1,
	MD_APPEND = 2,
};

static int bin_openFile(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 2);

	Heap *heap = Runtime_GetHeap(runtime);

	ParsedArgument pargs[2];
	if (!parseArgs(error, argv, argc, pargs, "si"))
		return -1;

	const char *path = pargs[0].as_string.data;
	int         mode = pargs[1].as_int;

	FILE *fp;
	{
		const char *mode2;

		switch(mode)
		{
			case MD_READ: 
			mode2 = "r"; 
			break;
				
			case MD_READ | MD_WRITE: 
			mode2 = "w+"; 
			break;

			case MD_READ | MD_APPEND:
			case MD_READ | MD_WRITE | MD_APPEND:
			mode2 = "a"; 
			break;

			default:
			UNREACHABLE;
			mode2 = NULL;
			break;
		}

		fp = fopen(path, mode2);

		if(fp == NULL) {
			const char *errdesc;
			switch(errno) {
				case EACCES:  errdesc = "Can't access file"; break;
				case EPERM:   errdesc = "Permission denied"; break;
				case EEXIST:  errdesc = "File or folder already exists"; break;
				case EISDIR:  errdesc = "Entity is a directory"; break;
				case ENOTDIR: errdesc = "Entity is not a directory"; break;
				case ELOOP:   errdesc = "Too many symbolic links"; break;
				case ENAMETOOLONG: errdesc = "Entity name is too long"; break;
				case ENFILE:  errdesc = "Open descriptors limit reached"; break;
				case ENOENT:  errdesc = "File or folder doesn't exist"; break;
				default:      errdesc = "Unexpected error"; break;
			}
			return returnValues(error, heap, rets, "ns", errdesc);
		}
	}
	return returnValues(error, heap, rets, "F", fp);
}

static int bin_read(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 3);

	// Arg 0: file
	// Arg 1: buffer
	// Arg 2: count

	ParsedArgument pargs[3];
	if (!parseArgs(error, argv, argc, pargs, "FB?i"))
		return -1;

	FILE  *stream;
	void  *dstptr;
	size_t dstlen;
	size_t count;

	stream = pargs[0].as_file;
	dstptr = pargs[1].as_buffer.data;
	dstlen = pargs[1].as_buffer.size;
	if (pargs[2].defined) {
		int n = pargs[2].as_int;
		if (n < 0) {
			Error_Report(error, ErrorType_RUNTIME, "Argument count must be a non-negative integer");
			return -1;
		}
		count = MIN((size_t) n, dstlen);
	} else
		count = (size_t) dstlen;

	size_t n = fread(dstptr, 1, count, stream);
	
	return returnValues2(error, runtime, rets, "i", n);
}

static int bin_write(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 3);

	// Arg 0: file
	// Arg 1: buffer
	// Arg 2: count

	ParsedArgument pargs[3];
	if (!parseArgs(error, argv, argc, pargs, "FB?i"))
		return -1;

	FILE  *stream;
	void  *srcptr;
	size_t srclen;
	size_t count;

	stream = pargs[0].as_file;
	srcptr = pargs[1].as_buffer.data;
	srclen = pargs[1].as_buffer.size;
	if (pargs[2].defined) {
		int n = pargs[2].as_int;
		if (n < 0) {
			Error_Report(error, ErrorType_RUNTIME, "Argument count must be a non-negative integer");
			return -1;
		}
		count = MIN((size_t) n, srclen);
	} else
		count = (int) srclen;

	size_t n = fwrite(srcptr, 1, count, stream);

	return returnValues2(error, runtime, rets, "i", n);
}

static int bin_openDir(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 1);

	// Arg 0: path

	if(!Object_IsString(argv[0]))
	{
		Error_Report(error, ErrorType_RUNTIME, "Expected first argument to be a string, but it's a %s", Object_GetName(argv[0]));
		return -1;
	}

	Heap *heap = Runtime_GetHeap(runtime);

	const char *path = Object_GetString(argv[0], NULL);
	ASSERT(path != NULL);

	DIR *dir = opendir(path);

	if(dir == NULL)
		return 0;

	Object *dob = Object_FromDIR(dir, heap, error);

	if(error->occurred)
	{
		(void) closedir(dir);
		return -1;
	}

	rets[0] = dob;
	return 1;
}

static int bin_nextDirItem(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 1);

	// Arg 0: path

	if(!Object_IsDir(argv[0]))
	{
		Error_Report(error, ErrorType_RUNTIME, "Expected first argument to be a directory, but it's a %s", Object_GetName(argv[0]));
		return -1;
	}

	DIR *dir = Object_GetDIR(argv[0]);
	ASSERT(dir != NULL);
	
	errno = 0;

	struct dirent *ent = readdir(dir);

	if(ent == NULL)
	{
		if(errno == 0)
			return 0; // Nothing left to read.

		// An error occurred.
		Error_Report(error, ErrorType_INTERNAL, "Failed to read directory item");
		return -1;
	}

	return returnValues2(error, runtime, rets, "s", ent->d_name);
}

StaticMapSlot bins_files[] = {
	{ "READ",        SM_INT, .as_int = MD_READ, },
	{ "WRITE",       SM_INT, .as_int = MD_WRITE, },
	{ "APPEND",      SM_INT, .as_int = MD_APPEND, },
	{ "openFile",    SM_FUNCT, .as_funct = bin_openFile, .argc = 2, },
	{ "openDir",     SM_FUNCT, .as_funct = bin_openDir,  .argc = 1, },
	{ "nextDirItem", SM_FUNCT, .as_funct = bin_nextDirItem, .argc = 1, },
	{ "read",        SM_FUNCT, .as_funct = bin_read,     .argc = 3, },
	{ "write",       SM_FUNCT, .as_funct = bin_write,    .argc = 3, },
	{ NULL, SM_END, {}, {} },
};