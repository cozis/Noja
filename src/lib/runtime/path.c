#include <string.h>
#include <unistd.h>
#include "path.h"
#include "../utils/defs.h"
#include "../utils/path.h"

// Returns the length written in buff (not considering the zero byte)
size_t Runtime_GetCurrentScriptFolder(Runtime *runtime, char *buff, size_t buffsize)
{
	const char *path = Runtime_GetCurrentScriptAbsolutePath(runtime);
	if(path == NULL) {
		if(getcwd(buff, buffsize) == NULL)
			return 0;
		size_t cwdlen = strlen(buff);
		if (buff[cwdlen-1] == '/')
			return cwdlen;
		else {
			if (cwdlen+1 >= buffsize)
				return 0;
			buff[cwdlen] = '/';
			return cwdlen+1;
		}
	}
	
	// This following block is a custom implementation
	// of [dirname], which doesn't write into the input
	// string and is way buggier. It will for sure give
	// problems in the future!!
	size_t dir_len;
	{
		// This is buggy code!!
		size_t path_len = strlen(path);
		ASSERT(path_len > 0); // Not empty
		ASSERT(Path_IsAbsolute(path)); // Is absolute
		ASSERT(path[path_len-1] != '/'); // Doesn't end with a slash.

		size_t popped = 0;
		while(path[path_len-1-popped] != '/')
			popped += 1;
		
		ASSERT(path_len > popped);

		dir_len = path_len - popped;
		
		ASSERT(dir_len < path_len);
		ASSERT(path[dir_len-1] == '/');
	}

	if(dir_len >= buffsize)
		return 0;

	memcpy(buff, path, dir_len);
	buff[dir_len] = '\0';
	return dir_len;
}

const char *Runtime_GetCurrentScriptAbsolutePath(Runtime *runtime)
{
	Executable *exe = Runtime_GetMostRecentExecutable(runtime);
	if (exe == NULL)
		return NULL;

	Source *src = Executable_GetSource(exe);
	if(src == NULL)
		return NULL;

	const char *path = Source_GetAbsolutePath(src);
	if(path == NULL)
		return NULL;

	ASSERT(path[0] != '\0');
	return path;
}
