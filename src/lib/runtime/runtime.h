
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

#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdio.h> // meh.. just for the definition of FILE.
#include "timing.h"
#include "../utils/error.h"
#include "../utils/stack.h"
#include "../objects/objects.h"
#include "../common/executable.h"

typedef struct xRuntime Runtime;
typedef struct xSnapshot Snapshot;

typedef struct {
    bool (*func)(Runtime*, void*);
    void  *data;
} RuntimeCallback;

typedef struct {
    bool time;
    size_t stack;
    RuntimeCallback callback;
} RuntimeConfig;

Runtime*	 Runtime_New(int heap_size, RuntimeConfig config);
Runtime*	 Runtime_New2(Heap *heap, bool free_it, RuntimeConfig config);
void 		 Runtime_Free(Runtime *runtime);
_Bool 		 Runtime_Pop(Runtime *runtime, Error *error, unsigned int n);
_Bool 		 Runtime_Push(Runtime *runtime, Error *error, Object *obj);
void         Runtime_Interrupt(Runtime *runtime);
Heap*		 Runtime_GetHeap(Runtime *runtime);
Stack*		 Runtime_GetStack(Runtime *runtime);
Object*		 Runtime_GetBuiltins(Runtime *runtime);
void 		 Runtime_SetBuiltins(Runtime *runtime, Object *builtins);
int 		 Runtime_GetCurrentIndex(Runtime *runtime);
Executable  *Runtime_GetCurrentExecutable(Runtime *runtime);
size_t       Runtime_GetCurrentScriptFolder(Runtime *runtime, char *buff, size_t buffsize);
const char  *Runtime_GetCurrentScriptAbsolutePath(Runtime *runtime);
TimingTable *Runtime_GetTimingTable(Runtime *runtime);
RuntimeConfig Runtime_GetDefaultConfigs();

Snapshot   *Snapshot_New(Runtime *runtime);
void 	    Snapshot_Free(Snapshot *snapshot);
void 	    Snapshot_Print(Snapshot *snapshot, FILE *fp);
int         run(Runtime *runtime, Error *error, Executable *exe, int index, Object *closure, Object **argv, int argc, Object *rets[static MAX_RETS]);

typedef enum {
    SM_END,
    SM_BOOL,
    SM_INT,
    SM_FLOAT,
    SM_FUNCT,
    SM_STRING,
    SM_SMAP,
    SM_NONE,
    SM_TYPE,
    SM_OBJECT,
} StaticMapSlotKind;

typedef struct StaticMapSlot StaticMapSlot;

struct StaticMapSlot {
    const char       *name;
    StaticMapSlotKind kind; 
    union {
        StaticMapSlot *as_smap;
        const char    *as_string;
        _Bool          as_bool;
        long long int  as_int;
        double         as_float;
        int          (*as_funct)(Runtime*, Object**, unsigned int, Object*[static MAX_RETS], Error*);
        TypeObject    *as_type;
        Object        *as_object;
    };
    union { int argc; int length; };
};

Object *Object_NewStaticMap(StaticMapSlot slots[], void (*initfn)(StaticMapSlot[]), Runtime *runt, Error *error);
Object *Object_FromNojaFunction(Runtime *runtime, const char *name, Executable *exe, int index, int argc, Object *closure, Heap *heap, Error *error);
Object *Object_FromNativeFunction(Runtime *runtime, int (*callback)(Runtime*, Object**, unsigned int, Object*[static MAX_RETS], Error*), int argc, Heap *heap, Error *error);

typedef struct {
    Error base;
    Runtime *runtime;
    Snapshot *snapshot;
} RuntimeError;

void RuntimeError_Init(RuntimeError *error, Runtime *runtime);
void RuntimeError_Free(RuntimeError *error);

#endif