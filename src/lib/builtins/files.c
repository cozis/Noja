
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
#include "../utils/defs.h"

enum {
	MD_READ = 0,
	MD_WRITE = 1,
	MD_APPEND = 2,
};

#include <errno.h>

static int bin_openFile(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 2);

	if(!Object_IsString(argv[0]))
	{
		Error_Report(error, 0, "Expected first argument to be a string, but it's a %s", Object_GetName(argv[0]));
		return -1;
	}

	if(!Object_IsInt(argv[1]))
	{
		Error_Report(error, 0, "Expected second argument to be an int, but it's a %s", Object_GetName(argv[1]));
		return -1;
	}

	Heap *heap = Runtime_GetHeap(runtime);

	const char *path = Object_ToString(argv[0], NULL, heap, error);

	if(error->occurred)
		return -1;

	int mode = Object_ToInt(argv[1], error);

	if(error->occurred)
		return -1;

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

			Object *o_none = Object_NewNone(heap, error);
			if(o_none == NULL)
				return -1;

			const char *errdesc;
			switch(errno) {
				case EACCES: errdesc = "Can't access file"; break;
				case EPERM: errdesc = "Permission denied"; break;
				case EEXIST: errdesc = "File or folder already exists"; break;
				case EISDIR: errdesc = "Entity is a directory"; break;
				case ENOTDIR: errdesc = "Entity is not a directory"; break;
				case ELOOP: errdesc = "Too many symbolic links"; break;
				case ENAMETOOLONG: errdesc = "Entity name is too long"; break;
				case ENFILE: errdesc = "Open descriptors limit reached"; break;
				case ENOENT: errdesc = "File or folder doesn't exist"; break;
				default: errdesc = "Unexpected error"; break;
			}

			Object *o_error = Object_FromString(errdesc, -1, heap, error);
			if(o_error == NULL)
				return -1;

			rets[0] = o_none;
			rets[1] = o_error;
			return 2;
		}
	}

	rets[0] = Object_FromStream(fp, heap, error);
	if(rets[0] == NULL)
		return -1;
	return 1;
}

static int bin_read(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 3);

	// Arg 0: file
	// Arg 1: buffer
	// Arg 2: count

	if(!Object_IsFile(argv[0]))
	{
		Error_Report(error, 0, "Expected first argument to be a file, but it's a %s", Object_GetName(argv[0]));
		return -1;
	}

	if(!Object_IsBuffer(argv[1]))
	{
		Error_Report(error, 0, "Expected second argument to be a buffer, but it's a %s", Object_GetName(argv[1]));
		return -1;
	}

	Heap *heap = Runtime_GetHeap(runtime);

	void *buff_addr;
	int   buff_size;
	
	buff_addr = Object_GetBufferAddrAndSize(argv[1], &buff_size, error);

	int read_size;

	if(Object_IsNone(argv[2]))
	{
		read_size = buff_size;
	}
	else if(Object_IsInt(argv[2]))
	{
		long long int temp = Object_ToInt(argv[2], error);

		if(error->occurred)
			return -1;

		read_size = temp; // TODO: Handle potential overflow.

		if(read_size > buff_size)
			read_size = buff_size;
	}
	else
	{
		Error_Report(error, 0, "Expected third argument to be an int or none, but it's a %s", Object_GetName(argv[0]));
		return -1;
	}

	FILE *fp = Object_ToStream(argv[0], error);
	
	if(fp == NULL)
		return -1;

	size_t n = fread(buff_addr, 1, read_size, fp);

	rets[0] = Object_FromInt(n, heap, error);
	if(rets[0] == NULL)
		return -1;
	return 1;
}

static int bin_write(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 3);

	// Arg 0: file
	// Arg 1: buffer
	// Arg 2: count

	if(!Object_IsFile(argv[0]))
	{
		Error_Report(error, 0, "Expected first argument to be a file, but it's a %s", Object_GetName(argv[0]));
		return -1;
	}

	if(!Object_IsBuffer(argv[1]))
	{
		Error_Report(error, 0, "Expected second argument to be a buffer, but it's a %s", Object_GetName(argv[1]));
		return -1;
	}

	Heap *heap = Runtime_GetHeap(runtime);

	void *buff_addr;
	int   buff_size;
	
	buff_addr = Object_GetBufferAddrAndSize(argv[1], &buff_size, error);

	int write_size;

	if(Object_IsNone(argv[2]))
	{
		write_size = buff_size;
	}
	else if(Object_IsInt(argv[2]))
	{
		long long int temp = Object_ToInt(argv[2], error);

		if(error->occurred)
			return -1;

		write_size = temp; // TODO: Handle potential overflow.

		if(write_size > buff_size)
			write_size = buff_size;
	}
	else
	{
		Error_Report(error, 0, "Expected third argument to be an int or none, but it's a %s", Object_GetName(argv[0]));
		return -1;
	}

	FILE *fp = Object_ToStream(argv[0], error);
	
	if(fp == NULL)
		return -1;

	size_t n = fwrite(buff_addr, 1, write_size, fp);

	rets[0] = Object_FromInt(n, heap, error);
	if(rets[0] == NULL)
		return -1;
	return 1;
}

static int bin_openDir(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	ASSERT(argc == 1);

	// Arg 0: path

	if(!Object_IsString(argv[0]))
	{
		Error_Report(error, 0, "Expected first argument to be a string, but it's a %s", Object_GetName(argv[0]));
		return -1;
	}

	Heap *heap = Runtime_GetHeap(runtime);

	const char *path = Object_ToString(argv[0], NULL, heap, error);
	
	if(error->occurred)
		return -1;

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
		Error_Report(error, 0, "Expected first argument to be a directory, but it's a %s", Object_GetName(argv[0]));
		return -1;
	}

	DIR *dir = Object_ToDIR(argv[0], error);

	if(error->occurred)
		return -1;

	ASSERT(dir != NULL);

	Heap *heap = Runtime_GetHeap(runtime);

	errno = 0;

	struct dirent *ent = readdir(dir);

	if(ent == NULL)
	{
		if(errno == 0)
			return 0; // Nothing left to read.

		// An error occurred.
		Error_Report(error, 1, "Failed to read directory item");
		return -1;
	}

	rets[0] = Object_FromString(ent->d_name, -1, heap, error);
	if(rets[0] == NULL)
		return -1;
	return 1;
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