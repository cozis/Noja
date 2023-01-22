
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

#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "run.h"
#include "runtime.h"
#include "utils/path.h"
#include "utils/defs.h"
#include "utils/stack.h"
#include "builtins/basic.h"

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

	FILE *stdin;
	FILE *stdout;
	FILE *stderr;

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
        .stdin  = stdin,
        .stdout = stdout,
        .stderr = stderr,
    };
}

Stack *Runtime_GetStack(Runtime *runtime)
{
	return runtime->stack;
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
	
	runtime->stdin  = config.stdin;
	runtime->stderr = config.stderr;
	runtime->stdout = config.stdout;

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

FILE *Runtime_GetErrorStream(Runtime *runtime)
{
	return runtime->stderr;
}

FILE *Runtime_GetOutputStream(Runtime *runtime)
{
	return runtime->stdout;
}

FILE *Runtime_GetInputStream(Runtime *runtime)
{
	return runtime->stdin;
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


bool Runtime_plugBuiltinsFromStaticMap(Runtime *runtime, StaticMapSlot *bin_table,  void (*bin_table_constructor)(StaticMapSlot*), Error *error)
{
	Object *object = Object_NewStaticMap(bin_table, bin_table_constructor, runtime, error);
	if (object == NULL)
		return false;
	return Runtime_plugBuiltins(runtime, object, error);
}

bool Runtime_plugBuiltinsFromSource(Runtime *runtime, Source *source, Error *error)
{
    Object *rets[8];
    int retc = runSource(runtime, source, rets, error);
    if(retc < 0)
        return false;
    if (retc == 0)
        return true;
    return Runtime_plugBuiltins(runtime, rets[0], error);
}

bool Runtime_plugBuiltinsFromFile(Runtime *runtime, const char *file, Error *error)
{
	Source *source = Source_FromFile(file, error);
    if (source == NULL)
        return false;
    
    bool result = Runtime_plugBuiltinsFromSource(runtime, source, error);

    Source_Free(source);
    return result;
}

bool Runtime_plugBuiltinsFromString(Runtime *runtime, const char *string, Error *error)
{
	Source *source = Source_FromString("<prelude>", string, -1, error);
    if (source == NULL)
        return false;
    
    bool result = Runtime_plugBuiltinsFromSource(runtime, source, error);

    Source_Free(source);
    return result;
}

bool Runtime_plugDefaultBuiltins(Runtime *runtime, Error *error)
{
	extern char start_noja[];
    return Runtime_plugBuiltinsFromStaticMap(runtime, bins_basic, bins_basic_init, error)
		&& Runtime_plugBuiltinsFromString(runtime, start_noja, error);
}

void Runtime_SerializeProfilingResultsToStream(Runtime *runtime, FILE *stream)
{
    TimingTable *table = Runtime_GetTimingTable(runtime);
    if (table == NULL)
        return; // Runtime wasn't profiling

    size_t count;
    const FunctionExecutionSummary *summary = TimingTable_getSummary(table, &count);

    for (size_t i = 0; i < count; i++) {
        if (summary[i].calls > 0) {
            fprintf(stream, "%20s - %s - %ld calls - %.2lfus\n",
                   summary[i].name,
                   Source_GetName(summary[i].src),
                   summary[i].calls,
                   summary[i].time * 1000000);
        }
    }
}

bool Runtime_SerializeProfilingResultsToFile(Runtime *runtime, const char *file)
{
    FILE *stream = fopen(file, "wb");
    if (stream == NULL)
        return false;
    Runtime_SerializeProfilingResultsToStream(runtime, stream);
    fclose(stream);
    return true;
}