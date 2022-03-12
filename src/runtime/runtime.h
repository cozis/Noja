#ifndef RUNTIME_H
#define RUNTIME_H
#include <stdio.h> // meh.. just for the definition of FILE.
#include "../utils/error.h"
#include "../utils/stack.h"
#include "../objects/objects.h"
#include "../common/executable.h"
typedef struct xRuntime Runtime;
typedef struct xSnapshot Snapshot;
Runtime*	Runtime_New(int stack_size, int heap_size, void *callback_userp, _Bool (*callback_addr)(Runtime*, void*));
Runtime*	Runtime_New2(int stack_size, Heap *heap, _Bool free_heap, void *callback_userp, _Bool (*callback_addr)(Runtime*, void*));
void 		Runtime_Free(Runtime *runtime);
_Bool 		Runtime_Pop(Runtime *runtime, Error *error, unsigned int n);
_Bool 		Runtime_Push(Runtime *runtime, Error *error, Object *obj);
Heap*		Runtime_GetHeap(Runtime *runtime);
Stack*		Runtime_GetStack(Runtime *runtime);
Object*		Runtime_GetBuiltins(Runtime *runtime);
void 		Runtime_SetBuiltins(Runtime *runtime, Object *builtins);
int 		Runtime_GetCurrentIndex(Runtime *runtime);
Executable *Runtime_GetCurrentExecutable(Runtime *runtime);
Snapshot   *Snapshot_New(Runtime *runtime);
void 	    Snapshot_Free(Snapshot *snapshot);
void 	    Snapshot_Print(Snapshot *snapshot, FILE *fp);
Object     *run(Runtime *runtime, Error *error, Executable *exe, int index, Object *closure, Object **argv, int argc);
#endif