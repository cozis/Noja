#include <stddef.h>
#include "runtime.h"
const char *Runtime_GetCurrentScriptAbsolutePath(Runtime *runtime);
size_t      Runtime_GetCurrentScriptFolder(Runtime *runtime, char *buff, size_t buffsize);
