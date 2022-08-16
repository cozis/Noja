#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "path.h"

_Bool Path_IsAbsolute(const char *path)
{
    return path[0] == '/';
}

const char *Path_MakeAbsolute(const char *path, char *buff, size_t buffsize)
{
    assert(path != NULL);

    if(Path_IsAbsolute(path))
        return path; // It's already absolute.

    if(getcwd(buff, buffsize) == NULL)
        return NULL;

    size_t written = strlen(buff);
    assert(buff[written-1] != '/');

    if(written+1 >= buffsize)
        return NULL; // No space for the / following the cwd.

    buff[written++] = '/';

    size_t n = strlen(path);
    if(written + n >= buffsize)
        return NULL;

    memcpy(buff + written, path, n);
    buff[written + n] = '\0';
    return buff;
}