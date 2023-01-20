
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
#include "executable.h"
#include "utils/error.h"
#include "utils/stack.h"
#include "objects/objects.h"

typedef struct xRuntime Runtime;
typedef struct xSnapshot Snapshot;
typedef struct StaticMapSlot StaticMapSlot;

typedef struct {
    bool (*func)(Runtime*, void*);
    void  *data;
} RuntimeCallback;

typedef struct {
    bool time;
    size_t heap;
    size_t stack;
    RuntimeCallback callback;
} RuntimeConfig;

Runtime*     Runtime_New(RuntimeConfig config);
void 		 Runtime_Free(Runtime *runtime);

bool Runtime_plugDefaultBuiltins(Runtime *runtime, Error *error);
bool Runtime_plugBuiltinsFromString(Runtime *runtime, const char *string, Error *error);
bool Runtime_plugBuiltinsFromFile(Runtime *runtime, const char *file, Error *error);
bool Runtime_plugBuiltinsFromStaticMap(Runtime *runtime, StaticMapSlot *bin_table,  void (*bin_table_constructor)(StaticMapSlot*), Error *error);
bool Runtime_plugBuiltinsFromSource(Runtime *runtime, Source *source, Error *error);
void Runtime_SerializeProfilingResultsToStream(Runtime *runtime, FILE *stream);
bool Runtime_SerializeProfilingResultsToFile(Runtime *runtime, const char *file);
Object *Runtime_Top(Runtime *runtime, int n);
bool Runtime_Pop (Runtime *runtime, Error *error, Object **p, unsigned int n);
bool Runtime_Push(Runtime *runtime, Error *error, Object *obj);
bool Runtime_PushFrame(Runtime *runtime, Error *error, Object *closure, Executable *exe, int index);
bool Runtime_PushNativeFrame(Runtime *runtime, Error *error);
bool Runtime_PushFailedFrame(Runtime *runtime, Error *error, Source *source, int offset);
bool Runtime_PopFrame(Runtime *runtime);
void Runtime_SetInstructionIndex(Runtime *runtime, int index);
bool Runtime_SetVariable(Runtime *runtime, Error *error, const char *name, Object *value);
bool Runtime_GetVariable(Runtime *runtime, Error *error, const char *name, Object **value);
Object *Runtime_GetLocals(Runtime *runtime);
Object *Runtime_GetClosure(Runtime *runtime);
size_t Runtime_GetFrameStackUsage(Runtime *runtime);
bool Runtime_WasInterrupted(Runtime *runtime);
const char *Runtime_GetCurrentScriptAbsolutePath(Runtime *runtime);
size_t      Runtime_GetCurrentScriptFolder(Runtime *runtime, char *buff, size_t buffsize);
RuntimeCallback Runtime_GetCallback(Runtime *runtime);
bool Runtime_CollectGarbage(Runtime *runtime, Error *error);
void Runtime_PrintStackTrace(Runtime *runtime, FILE *stream);
void         Runtime_Interrupt(Runtime *runtime);
Heap*		 Runtime_GetHeap(Runtime *runtime);
Stack*		 Runtime_GetStack(Runtime *runtime);
int 		 Runtime_GetCurrentIndex(Runtime *runtime);
Executable  *Runtime_GetCurrentExecutable(Runtime *runtime);
Executable  *Runtime_GetMostRecentExecutable(Runtime *runtime);
size_t       Runtime_GetCurrentScriptFolder(Runtime *runtime, char *buff, size_t buffsize);
const char  *Runtime_GetCurrentScriptAbsolutePath(Runtime *runtime);
TimingTable *Runtime_GetTimingTable(Runtime *runtime);
RuntimeConfig Runtime_GetDefaultConfigs();

bool Runtime_plugBuiltins(Runtime *runtime, Object *object, Error *error);
int  Runtime_runSource(Runtime *runtime, Source *source, Object *rets[static MAX_RETS], Error *error);
int  Runtime_runBytecodeSource(Runtime *runtime, Source *source, Object *rets[static MAX_RETS], Error *error);

Snapshot   *Snapshot_New(Runtime *runtime);
void 	    Snapshot_Free(Snapshot *snapshot);
void 	    Snapshot_Print(Snapshot *snapshot, FILE *fp);

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
void RuntimeError_Print(RuntimeError *error, ErrorType type_if_unspecified, FILE *stream);

#endif