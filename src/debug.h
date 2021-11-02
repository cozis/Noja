#ifndef DEBUG_H
#define DEBUG_H
#include "runtime/runtime.h"
typedef struct xDebug Debug;
Debug *Debug_New();
void   Debug_Free(Debug *dbg);
_Bool  Debug_Callback(Runtime *runtime, void *userp);
#endif