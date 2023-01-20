
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

#include <stdbool.h>
#include "runtime.h"
#include "../utils/path.h"
#include "../utils/defs.h"
#include "../utils/stack.h"

#define MAX_FRAME_STACK 16
#define MAX_FRAMES 16

typedef enum {
	FrameType_NATIVE,
	FrameType_NORMAL,
	FrameType_FAILED,
} FrameType;

typedef struct Frame Frame;
struct Frame {
	Frame    *prev;
	FrameType type;
};

typedef struct {
	Frame base;
	Object *locals;
	Object *closure;
	Executable *exe;
	int index, used;
} NormalFrame;

typedef struct {
	Frame base;
} NativeFrame;

typedef struct {
	Frame base;
	Source *source;
	size_t  offset;
} FailedFrame;

struct xRuntime {
	bool interrupt;
	RuntimeCallback callback;
	_Bool free_heap;
	Object *builtins;
	int    depth;
	Frame *frame;
	Stack *stack;
	Heap  *heap;
	TimingTable *timing;

	FailedFrame failed_frame;
};

bool Runtime_plugBuiltins(Runtime *runtime, Object *object, Error *error)
{
	if (runtime->builtins == NULL) {
		runtime->builtins = object;
	} else {
		Heap *heap = Runtime_GetHeap(runtime);
		Object *old_builtins;
		Object *new_builtins;

		old_builtins = runtime->builtins;
		new_builtins = Object_NewClosure(object, old_builtins, heap, error);
		if (new_builtins == NULL)
			return false;

		runtime->builtins = new_builtins;
	}

	return true;
}

RuntimeConfig Runtime_GetDefaultConfigs()
{
    return (RuntimeConfig) {
    	.heap  = 1024*1024,
        .stack = 1024,
        .callback = { .func = NULL, .data = NULL },
        .time = false,
    };
}

Stack *Runtime_GetStack(Runtime *runtime)
{
	return Stack_Copy(runtime->stack, 1);
}

Heap *Runtime_GetHeap(Runtime *runtime)
{
	return runtime->heap;
}

int Runtime_GetCurrentIndex(Runtime *runtime)
{
	Frame *frame = runtime->frame;
	if(frame == NULL || frame->type != FrameType_NORMAL)
		return -1;
	return ((NormalFrame*) frame)->index;
}

static Source *getFrameSource(Frame *frame)
{
	switch (frame->type) {
		case FrameType_NORMAL: return Executable_GetSource(((NormalFrame*) frame)->exe);
		case FrameType_NATIVE: return NULL;
		case FrameType_FAILED: return ((FailedFrame*) frame)->source;
	}
	UNREACHABLE;
	return NULL;
}

static int getFrameOffset(Frame *frame)
{
	switch (frame->type) {
		case FrameType_NORMAL: 
		{
			NormalFrame *normal_frame = (NormalFrame*) frame;
			return Executable_GetInstrOffset(normal_frame->exe, normal_frame->index);
		}

		case FrameType_NATIVE: return -1;
		case FrameType_FAILED: return ((FailedFrame*) frame)->offset;
	}
	UNREACHABLE;
	return -1;
}

static void printFrame(Frame *frame, int depth, FILE *stream)
{
	Source *source = getFrameSource(frame);
	int     offset = getFrameOffset(frame);

	int line;
	const char *name;


	if (source == NULL)
		// Executable has no associate source object
		name = "(no source)";
	else {
		name = Source_GetName(source);
		if (name == NULL)
			// Executable has a source but the source
			// doesn't have a name.
			name = "(unnamed)";
	}
	
	if(source == NULL || offset < 0)
		line = 0;
	else {
		line = 1;

		const char *body = Source_GetBody(source);

		int i = 0;
	
		while(i < offset)
		{
			if(body[i] == '\n')
				line += 1;

			i += 1;
		}
	}

	if(line == 0)
		fprintf(stream, "\t#%d %s\n", depth, name);
	else
		fprintf(stream, "\t#%d %s:%d\n", depth, name, line);
}

void Runtime_PrintStackTrace(Runtime *runtime, FILE *stream)
{
	fprintf(stream, "Stack trace:\n");
	Frame *frame = runtime->frame;
	if (frame == NULL)
		fprintf(stderr, "\t(No active frames)\n");
	else {
		size_t depth = 0;
		do {
			printFrame(frame, depth, stream);
			frame = frame->prev;
			depth++;
		} while (frame != NULL);
	}
}

Executable *Runtime_GetMostRecentExecutable(Runtime *runtime)
{
	Frame *frame = runtime->frame;
	while (frame != NULL && frame->type != FrameType_NORMAL)
		frame = frame->prev;
	if (frame == NULL)
		return NULL;
	return ((NormalFrame*) frame)->exe;
}

Executable *Runtime_GetCurrentExecutable(Runtime *runtime)
{
	if(runtime->depth == 0 || runtime->frame->type != FrameType_NORMAL)
		return 	NULL;
	
	return ((NormalFrame*) runtime->frame)->exe;
}

TimingTable *Runtime_GetTimingTable(Runtime *runtime)
{
	return runtime->timing;
}

void Runtime_Interrupt(Runtime *runtime)
{
	runtime->interrupt = true;
}

Runtime *Runtime_New(RuntimeConfig config)
{
	Runtime *runtime = malloc(sizeof(Runtime));
	if (runtime == NULL)
		return NULL;

	runtime->heap = Heap_New(config.heap);
	if(runtime->heap == NULL) {
		free(runtime);
		return NULL;
	}

	runtime->stack = Stack_New(config.stack);
	if(runtime->stack == NULL) {
		Heap_Free(runtime->heap);
		free(runtime);
		return NULL;
	}

	TimingTable *timing_table = NULL;
	if (config.time) {
		timing_table = TimingTable_new();
		if (timing_table == NULL) {
			Stack_Free(runtime->stack);
			Heap_Free(runtime->heap);
			free(runtime);
			return NULL;
		}
	}

	runtime->interrupt = false;
	runtime->timing = timing_table;
	runtime->callback = config.callback;
	runtime->builtins = NULL;
	runtime->frame = NULL;
	runtime->depth = 0;

	return runtime;
}

void Runtime_Free(Runtime *runtime)
{
	while (runtime->frame != NULL)
		Runtime_PopFrame(runtime);
	if (runtime->timing != NULL)
		TimingTable_free(runtime->timing);
	Stack_Free(runtime->stack);
	Heap_Free(runtime->heap);
	free(runtime);
}

_Bool Runtime_Push(Runtime *runtime, Error *error, Object *obj)
{
	ASSERT(runtime != NULL);
	ASSERT(error != NULL);
	ASSERT(obj != NULL);

	Frame *frame = runtime->frame;
	if(frame == NULL)
	{
		Error_Report(error, ErrorType_RUNTIME, "There are no frames on the stack");
		return 0;
	}
	if (frame->type == FrameType_NATIVE) {
		Error_Report(error, ErrorType_INTERNAL, "Can't push on the stack from a native function");
		return 0;
	}
	if (frame->type == FrameType_FAILED) {
		Error_Report(error, ErrorType_INTERNAL, "Can't push on the stack after a compilation error");
		return 0;
	}

	ASSERT(frame->type == FrameType_NORMAL);
	NormalFrame *normal_frame = (NormalFrame*) frame;

	if(normal_frame->used == MAX_FRAME_STACK)
	{
		Error_Report(error, ErrorType_RUNTIME, "Frame stack limit of %d reached", MAX_FRAME_STACK);
		return 0;
	}

	if(!Stack_Push(runtime->stack, obj))
	{
		Error_Report(error, ErrorType_RUNTIME, "Out of stack");
		return 0;
	}

	normal_frame->used++;
	return 1;
}

_Bool Runtime_Pop(Runtime *runtime, Error *error, Object **p, unsigned int n)
{
	ASSERT(runtime != NULL);
	ASSERT(error != NULL);

	Frame *frame = runtime->frame;

	if (frame == NULL)
	{
		Error_Report(error, ErrorType_RUNTIME, "There are no frames on the stack");
		return 0;
	}
	if (frame->type == FrameType_NATIVE) {
		Error_Report(error, ErrorType_INTERNAL, "Can't pop from the stack from a native function");
		return 0;
	}
	if (frame->type == FrameType_FAILED) {
		Error_Report(error, ErrorType_INTERNAL, "Can't pop from the stack after a compilation error");
		return 0;
	}

	ASSERT(frame->type == FrameType_NORMAL);
	NormalFrame *normal_frame = (NormalFrame*) frame;

	ASSERT(normal_frame->used >= 0);
	if((unsigned int) normal_frame->used < n)
	{
		Error_Report(error, ErrorType_RUNTIME, "Frame has not enough values on the stack");
		return 0;
	}

	if (p != NULL)
		for (unsigned int i = 0; i < n; i++)
			p[i] = Stack_Top(runtime->stack, -i);

	// The frame has something on the stack,
	// this means that the stack isn't empty
	// and popping won't fail.
	(void) Stack_Pop(runtime->stack, n);

	normal_frame->used -= n;

	ASSERT(normal_frame->used >= 0);
	
	return 1;
}

Object *Runtime_Top(Runtime *runtime, int n)
{
	if (runtime->frame->type != FrameType_NORMAL)
		return NULL;

	NormalFrame *normal_frame = (NormalFrame*) runtime->frame;

	if (normal_frame->used + n <= 0)
		return NULL;

	Object *top = Stack_Top(runtime->stack, n);
	ASSERT(top != NULL);
	
	return top;
}

void Runtime_SetInstructionIndex(Runtime *runtime, int index)
{
	ASSERT(runtime->frame->type == FrameType_NORMAL);
	((NormalFrame*) runtime->frame)->index = index;
}

bool Runtime_SetVariable(Runtime *runtime, Error *error, const char *name, Object *value) 
{
	Frame *frame = runtime->frame;
	if (frame == NULL) {
		Error_Report(error, ErrorType_INTERNAL, "Can't assign a variable while there's no active frame");
		return false;
	}
	if (frame->type == FrameType_NATIVE) {
		Error_Report(error, ErrorType_INTERNAL, "Can't set a variable from a native function");
		return 0;
	}
	if (frame->type == FrameType_FAILED) {
		Error_Report(error, ErrorType_INTERNAL, "Can't set a variable after a compilation error");
		return 0;
	}

	ASSERT(frame->type == FrameType_NORMAL);
	NormalFrame *normal_frame = (NormalFrame*) frame;

	Heap *heap = Runtime_GetHeap(runtime);
	Object *key = Object_FromString(name, -1, heap, error);
	if(key == NULL)
		return false;

	return Object_Insert(normal_frame->locals, key, value, heap, error);
}

bool Runtime_GetVariable(Runtime *runtime, Error *error, const char *name, Object **value)
{
	ASSERT(value != NULL);

	Frame *frame = runtime->frame;
	if (frame == NULL) {
		Error_Report(error, ErrorType_INTERNAL, "Can't load a variable while there is no active frame");
		return 0;
	}
	if (frame->type == FrameType_NATIVE) {
		Error_Report(error, ErrorType_INTERNAL, "Can't load a variable from a native function");
		return 0;
	}
	if (frame->type == FrameType_FAILED) {
		Error_Report(error, ErrorType_INTERNAL, "Can't load a variable after a compilation error");
		return 0;
	}

	ASSERT(frame->type == FrameType_NORMAL);
	NormalFrame *normal_frame = (NormalFrame*) frame;

	Heap *heap = Runtime_GetHeap(runtime);
	Object *key = Object_FromString(name, -1, heap, error);
	if(key == NULL)
		return false;

	Object *locations[] = {
		normal_frame->locals,
		normal_frame->closure,
		runtime->builtins,
	};
		
	Object *obj = NULL;

	for(int p = 0; obj == NULL && (unsigned int) p < sizeof(locations)/sizeof(locations[0]); p += 1)
	{
		if(locations[p] == NULL)
			continue;

		obj = Object_Select(locations[p], key, heap, error);
	}

	*value = obj;
	return true;
}

Object *Runtime_GetClosure(Runtime *runtime)
{
	ASSERT(runtime->frame != NULL && runtime->frame->type == FrameType_NORMAL);
	NormalFrame *normal_frame = (NormalFrame*) runtime->frame;
	return normal_frame->closure;
}

Object *Runtime_GetLocals(Runtime *runtime)
{
	ASSERT(runtime->frame != NULL && runtime->frame->type == FrameType_NORMAL);
	NormalFrame *normal_frame = (NormalFrame*) runtime->frame;
	return normal_frame->locals;
}

size_t Runtime_GetFrameStackUsage(Runtime *runtime)
{
	if (runtime->frame->type != FrameType_NORMAL)
		return 0;
	return ((NormalFrame*) runtime->frame)->used;
}

static bool appendFrame(Runtime *runtime, Error *error, Frame *frame)
{
	if(runtime->depth == MAX_FRAMES) {
		Error_Report(error, ErrorType_INTERNAL, "Maximum nested call limit of %d was reached", MAX_FRAMES);
		return false;
	}
	frame->prev = runtime->frame;
	runtime->frame = frame;
	runtime->depth++;
	return true;
}

bool Runtime_PushFailedFrame(Runtime *runtime, Error *error, Source *source, int offset)
{
	FailedFrame *failed_frame = &runtime->failed_frame;

	Source *source_copy = Source_Copy(source);
	if (source_copy == NULL) {
		Error_Report(error, ErrorType_INTERNAL, "Failed to copy source object");
		return false;
	}

	failed_frame->base.type = FrameType_FAILED;
	failed_frame->base.prev = NULL;
	failed_frame->source = source;
	failed_frame->offset = offset;

	if (!appendFrame(runtime, error, (Frame*) failed_frame)) {
		Source_Free(source_copy);
		return false;
	}
	return true;
}

bool Runtime_PushNativeFrame(Runtime *runtime, Error *error)
{
	NativeFrame *native_frame = malloc(sizeof(NativeFrame));
	if (native_frame == NULL) {
		Error_Report(error, ErrorType_INTERNAL, "Out of memory");
		return false;
	}
	
	native_frame->base.type = FrameType_NATIVE;
	native_frame->base.prev = NULL;

	if (!appendFrame(runtime, error, (Frame*) native_frame)) {
		free(native_frame);
		return false;
	}
	return true;
}

bool Runtime_PushFrame(Runtime *runtime, Error *error, Object *closure, Executable *exe, int index)
{
	NormalFrame *frame = malloc(sizeof(NormalFrame));
	if (frame == NULL) {
		Error_Report(error, ErrorType_INTERNAL, "Out of memory");
		return false;
	}
	
	Object *locals = Object_NewMap(-1, runtime->heap, error);
	if (locals == NULL) {
		free(frame);
		return false;
	}

	Executable *exe_copy = Executable_Copy(exe);
	if (exe_copy == NULL) {
		free(frame);
		Error_Report(error, ErrorType_INTERNAL, "Failed to copy executable");
		return false;
	}

	frame->base.type = FrameType_NORMAL;
	frame->base.prev = NULL;
	frame->closure = closure;
	frame->exe = exe_copy;
	frame->index = index;
	frame->used = 0;
	frame->locals = locals;

	if (!appendFrame(runtime, error, (Frame*) frame)) {
		free(frame);
		Executable_Free(exe_copy);
		return false;
	}
	return true;
}

bool Runtime_PopFrame(Runtime *runtime)
{
	Frame *frame = runtime->frame;
	if (frame == NULL)
		return false;
	runtime->frame = frame->prev;
	runtime->depth--;
	switch(frame->type) {
		case FrameType_NORMAL: {
			NormalFrame *normal_frame = (NormalFrame*) frame;
			Stack_Pop(runtime->stack, normal_frame->used);
			Executable_Free(normal_frame->exe);
			free(frame);
			break;
		}
		case FrameType_NATIVE: {
			NativeFrame *native_frame = (NativeFrame*) frame;
			UNUSED(native_frame);
			free(frame);
			break;
		}
		case FrameType_FAILED: {
			FailedFrame *failed_frame = (FailedFrame*) frame;
			Source_Free(failed_frame->source);
			break;
		}
	}
	return true;
}

RuntimeCallback Runtime_GetCallback(Runtime *runtime)
{
	return runtime->callback;
}

bool Runtime_WasInterrupted(Runtime *runtime)
{
	return runtime->interrupt;
}

bool Runtime_CollectGarbage(Runtime *runtime, Error *error)
{
	Frame *frame = runtime->frame;
	Heap *heap = runtime->heap;

	if(!Heap_StartCollection(heap, error))
		return 0;

	Heap_CollectReference(&runtime->builtins,  runtime->heap);

	while(frame)
	{
		if (frame->type == FrameType_NORMAL) {
			NormalFrame *normal_frame = (NormalFrame*) frame;
			Heap_CollectReference(&normal_frame->locals,  heap);
			Heap_CollectReference(&normal_frame->closure, heap);
		}
		frame = frame->prev;
	}

	Stack *stack = Runtime_GetStack(runtime);
	for(unsigned int i = 0; i < Stack_Size(stack); i += 1)
	{
		Object **ref = (Object**) Stack_TopRef(stack, -i);
		ASSERT(ref != NULL);

		Heap_CollectReference(ref, runtime->heap);
	}

	return Heap_StopCollection(runtime->heap);
}