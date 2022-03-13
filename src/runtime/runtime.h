
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
} StaticMapSlotKind;

typedef struct StaticMapSlot StaticMapSlot;

struct StaticMapSlot {
    const char       *name;
    StaticMapSlotKind kind; 
    union {
        const StaticMapSlot *as_smap;
        const char          *as_string;
        _Bool                as_bool;
        long long int        as_int;
        double               as_float;
        Object            *(*as_funct)(Runtime*, Object**, unsigned int, Error*);
        TypeObject          *as_type;
    };
    union { int argc; int length; };
};

Object *Object_NewStaticMap(const StaticMapSlot *slots, Runtime *runt, Error *error);
Object *Object_FromNojaFunction(Runtime *runtime, Executable *exe, int index, int argc, Object *closure, Heap *heap, Error *error);
Object *Object_FromNativeFunction(Runtime *runtime, Object *(*callback)(Runtime*, Object**, unsigned int, Error*), int argc, Heap *heap, Error *error);

typedef struct {
    Error base;
    Runtime *runtime;
    Snapshot *snapshot;
} RuntimeError;

void RuntimeError_Init(RuntimeError *error, Runtime *runtime);
void RuntimeError_Free(RuntimeError *error);

#endif