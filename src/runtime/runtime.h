#ifndef RUNTIME_H
#define RUNTIME_H
#include <stdio.h> // meh.. just for the definition of FILE.
#include "../utils/error.h"
#include "../utils/stack.h"
#include "../objects/objects.h"
#include "../common/executable.h"

typedef struct xRuntime Runtime;
Runtime*	Runtime_New(int stack_size, int heap_size, void *callback_userp, _Bool (*callback_addr)(Runtime*, void*));
void 		Runtime_Free(Runtime *runtime);
_Bool 		Runtime_Pop(Runtime *runtime, Error *error, unsigned int n);
_Bool 		Runtime_Push(Runtime *runtime, Error *error, Object *obj);
Heap*		Runtime_GetHeap(Runtime *runtime);
Stack*		Runtime_GetStack(Runtime *runtime);
Object*		Runtime_GetBuiltins(Runtime *runtime);
void 		Runtime_SetBuiltins(Runtime *runtime, Object *builtins);
int 		Runtime_GetCurrentIndex(Runtime *runtime);
Executable *Runtime_GetCurrentExecutable(Runtime *runtime);

typedef struct xSnapshot Snapshot;
Snapshot *Snapshot_New(Runtime *runtime);
void 	  Snapshot_Free(Snapshot *snapshot);
void 	  Snapshot_Print(Snapshot *snapshot, FILE *fp);

Object *run(Runtime *runtime, Error *error, Executable *exe, int index, Object *closure, Object **argv, int argc);

Object*	Object_FromNativeFunction(Runtime *runtime, Object *(*callback)(Runtime*, Object**, unsigned int, Error*), int argc, Heap *heap, Error *error);
Object *Object_FromNojaFunction(Runtime *runtime, Executable *exe, int index, int argc, Object *closure, Heap *heap, Error *error);
#endif