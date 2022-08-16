
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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../utils/path.h"
#include "../utils/defs.h"
#include "../utils/stack.h"
#include "runtime.h"

#define MAX_FRAME_STACK 16
#define MAX_FRAMES 16

typedef struct xFrame Frame;
struct xFrame {
	Frame  *prev;
	Object *locals;
	Object *closure;
	Executable *exe;
	int index, used;
};

struct xRuntime {
	void *callback_userp;
	_Bool (*callback_addr)(Runtime*, void*);
	_Bool free_heap;
	Object *builtins;
	int    depth;
	Frame *frame;
	Stack *stack;
	Heap  *heap;
};


// Returns the length written in buff (not considering the zero byte)
size_t Runtime_GetCurrentScriptFolder(Runtime *runtime, char *buff, size_t buffsize)
{
	const char *path = Runtime_GetCurrentScriptAbsolutePath(runtime);
	if(path == NULL) {
		if(getcwd(buff, sizeof(buffsize)) == NULL)
			return 0;
		return strlen(buff);
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
	Executable *exe = Runtime_GetCurrentExecutable(runtime);
	ASSERT(exe != NULL);

	Source *src = Executable_GetSource(exe);
	if(src == NULL)
		return NULL;

	const char *path = Source_GetAbsolutePath(src);
	if(path == NULL)
		return NULL;

	ASSERT(path[0] != '\0');
	return path;
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
	if(runtime->depth == 0)
		return -1;
	else
		return runtime->frame->index;
}

Executable *Runtime_GetCurrentExecutable(Runtime *runtime)
{
	if(runtime->depth == 0)
		return 	NULL;
	else
		return runtime->frame->exe;
}

Runtime *Runtime_New2(int stack_size, Heap *heap, _Bool free_heap, void *callback_userp, _Bool (*callback_addr)(Runtime*, void*))
{
	if(stack_size < 0)
		stack_size = 1024;

	Runtime *runtime = malloc(sizeof(Runtime));

	if(runtime != NULL)
	{
		runtime->heap = heap;
		runtime->stack = Stack_New(stack_size);

		if(runtime->stack == NULL)
		{
			Heap_Free(runtime->heap);
			free(runtime);
		}

		runtime->free_heap = free_heap;
		runtime->callback_userp = callback_userp;
		runtime->callback_addr = callback_addr;
		runtime->builtins = NULL;
		runtime->frame = NULL;
		runtime->depth = 0;
	}

	return runtime;
}

Runtime *Runtime_New(int stack_size, int heap_size, void *callback_userp, _Bool (*callback_addr)(Runtime*, void*))
{
	if(heap_size < 0)
		heap_size = 65536;

	Heap *heap = Heap_New(heap_size);

	if(heap == NULL)
		return NULL;

	return Runtime_New2(stack_size, heap, 1, callback_userp, callback_addr);
}

void Runtime_Free(Runtime *runtime)
{
	if(runtime->free_heap)
		Heap_Free(runtime->heap);
	Stack_Free(runtime->stack);
	free(runtime);
}

Object *Runtime_GetBuiltins(Runtime *runtime)
{
	return runtime->builtins;
}

void Runtime_SetBuiltins(Runtime *runtime, Object *builtins)
{
	runtime->builtins = builtins;
}

_Bool Runtime_Push(Runtime *runtime, Error *error, Object *obj)
{
	ASSERT(runtime != NULL);
	ASSERT(error != NULL);
	ASSERT(obj != NULL);

	if(runtime->depth == 0)
	{
		Error_Report(error, 0, "There are no frames on the stack");
		return 0;
	}

	ASSERT(runtime->frame->used <= MAX_FRAME_STACK);
	
	if(runtime->frame->used == MAX_FRAME_STACK)
	{
		Error_Report(error, 0, "Frame stack limit of %d reached", MAX_FRAME_STACK);
		return 0;
	}

	if(!Stack_Push(runtime->stack, obj))
	{
		Error_Report(error, 0, "Out of stack");
		return 0;	
	}

	runtime->frame->used += 1;
	return 1;
}

_Bool Runtime_Pop(Runtime *runtime, Error *error, unsigned int n)
{
	ASSERT(runtime != NULL);
	ASSERT(error != NULL);

	if(runtime->depth == 0)
	{
		Error_Report(error, 0, "There are no frames on the stack");
		return 0;
	}

	ASSERT(runtime->frame->used >= 0);

	if((unsigned int) runtime->frame->used < n)
	{
		Error_Report(error, 0, "Frame has not enough values on the stack");
		return 0;
	}

	// The frame has something on the stack,
	// this means that the stack isn't empty
	// and popping won't fail.
	(void) Stack_Pop(runtime->stack, n);

	runtime->frame->used -= n;

	ASSERT(runtime->frame->used >= 0);
	
	return 1;
}

typedef struct {
	Executable *exe;
	int 		index;
} SnapshotNode;

struct xSnapshot {
	int depth;
	SnapshotNode nodes[];
};

Snapshot *Snapshot_New(Runtime *runtime)
{
	ASSERT(runtime->depth >= 0);

	Snapshot *snapshot = malloc(sizeof(Snapshot) + sizeof(SnapshotNode) * runtime->depth);

	if(snapshot == NULL)
		return NULL;

	{
		Frame *f = runtime->frame;

		snapshot->depth = 0;

		while(snapshot->depth < runtime->depth)
		{
			ASSERT(f != NULL);

			SnapshotNode *node = snapshot->nodes + snapshot->depth;

			node->exe   = Executable_Copy(f->exe);
			node->index = f->index;

			if(node->exe == NULL)
				goto abort;

			f = f->prev;
			snapshot->depth += 1;
		}

		ASSERT(f == NULL);
	}

	return snapshot;

abort:
	Snapshot_Free(snapshot);
	return NULL;
}

void Snapshot_Free(Snapshot *snapshot)
{
	for(int i = 0; i < snapshot->depth; i += 1)
	{
		Executable *exe = snapshot->nodes[i].exe;		
		Executable_Free(exe);
	}
	free(snapshot);
}

void Snapshot_Print(Snapshot *snapshot, FILE *fp)
{
	ASSERT(snapshot != NULL);
	ASSERT(fp != NULL);

	fprintf(fp, "Stack trace:\n");

	for(int i = 0; i < snapshot->depth; i += 1)
	{
		SnapshotNode node = snapshot->nodes[i];

		Executable *exe = node.exe;
		Source     *src = Executable_GetSource(exe);

		const char *name;
		{
			name = NULL;

			if(src != NULL) 
				name = Source_GetName(src);

			if(name == NULL)
				name = "(unnamed)";
		}

		int line;
		{
			if(src == NULL)
				line = 0;
			else
				{
					line = 1;

					const char *body = Source_GetBody(src);
					int offset = Executable_GetInstrOffset(exe, node.index);

					int i = 0;
				
					while(i < offset)
					{
						if(body[i] == '\n')
							line += 1;

						i += 1;
					}
				}
		}

		if(line == 0)
			fprintf(fp, "\t#%d %s\n", i, name);
		else
			fprintf(fp, "\t#%d %s:%d\n", i, name, line);
	}

	//fprintf(fp, "  (Snapshot can't be printed yet)\n");
}

static Object *do_math_op(Object *lop, Object *rop, Opcode opcode, Heap *heap, Error *error)
{
	ASSERT(lop != NULL);
	ASSERT(rop != NULL);

	#define APPLY(x, y, z, id) 							\
		switch(opcode)									\
		{												\
			case OPCODE_ADD: (z) = (x) + (y); break;	\
			case OPCODE_SUB: (z) = (x) - (y); break;	\
			case OPCODE_MUL: (z) = (x) * (y); break;	\
			case OPCODE_DIV: 							\
			if((y) == 0) 								\
			{ 											\
				Error_Report(error, 0, "Division by zero"); \
				return NULL; 							\
			} 											\
			(z) = (x) / (y); 							\
			break;										\
			default: UNREACHABLE; break;				\
		}

	Object *res;

	if(Object_IsInt(lop))
	{
		long long int raw_lop = Object_ToInt(lop, error);

		if(error->occurred)
			return NULL;

		if(Object_IsInt(rop))
		{
			// int + int
			long long int raw_rop = Object_ToInt(rop, error);

			if(error->occurred)
				return NULL;

			long long int raw_res = 0;

			APPLY(raw_lop, raw_rop, raw_res, id)

			res = Object_FromInt(raw_res, heap, error);
		}
		else if(Object_IsFloat(rop))
		{
			// int + float
			double raw_rop = Object_ToFloat(rop, error);

			if(error->occurred)
				return NULL;

			double raw_res = 0;

			APPLY((double) raw_lop, raw_rop, raw_res, id)

			res = Object_FromFloat(raw_res, heap, error);
		}
		else
		{
			Error_Report(error, 0, "Arithmetic operation on a non-numeric object");
			return NULL;
		}
	}
	else if(Object_IsFloat(lop))
	{
		double raw_lop = Object_ToFloat(lop, error);

		if(error->occurred)
			return NULL;

		if(Object_IsInt(rop))
		{
			// float + int
			long long int raw_rop = Object_ToInt(rop, error);

			if(error->occurred)
				return NULL;

			double raw_res = 0;

			APPLY(raw_lop, (double) raw_rop, raw_res, id)

			res = Object_FromFloat(raw_res, heap, error);
		}
		else if(Object_IsFloat(rop))
		{
			// float + float
			double raw_rop = Object_ToFloat(rop, error);

			if(error->occurred)
				return NULL;

			double raw_res = 0;

			APPLY(raw_lop, raw_rop, raw_res, id)

			res = Object_FromFloat(raw_res, heap, error);
		}
		else
		{
			Error_Report(error, 0, "Arithmetic operation on a non-numeric object");
			return NULL;
		}
	}
	else
	{
		Error_Report(error, 0, "Arithmetic operation on a non-numeric object");
		return NULL;
	}

	#undef APPLY

	return res;
}

static Object *do_relational_op(Object *lop, Object *rop, Opcode opcode, Heap *heap, Error *error)
{
	ASSERT(lop != NULL);
	ASSERT(rop != NULL);

	#define APPLY(x, y, z, id) 							\
		switch(opcode)									\
		{												\
			case OPCODE_LSS: (z) = (x) <  (y); break;	\
			case OPCODE_GRT: (z) = (x) >  (y); break;	\
			case OPCODE_LEQ: (z) = (x) <= (y); break;	\
			case OPCODE_GEQ: (z) = (x) >= (y); break;	\
			default: UNREACHABLE; break;				\
		}

	_Bool res = 0;

	if(Object_IsInt(lop))
	{
		long long int raw_lop = Object_ToInt(lop, error);

		if(error->occurred)
			return NULL;

		if(Object_IsInt(rop))
		{
			// int + int
			long long int raw_rop = Object_ToInt(rop, error);

			if(error->occurred)
				return NULL;

			APPLY(raw_lop, raw_rop, res, id)
		}
		else if(Object_IsFloat(rop))
		{
			// int + float
			double raw_rop = Object_ToFloat(rop, error);

			if(error->occurred)
				return NULL;

			APPLY((double) raw_lop, raw_rop, res, id)
		}
		else
		{
			Error_Report(error, 0, "Relational operation on a non-numeric object");
			return NULL;
		}
	}
	else if(Object_IsFloat(lop))
	{
		double raw_lop = Object_ToFloat(lop, error);

		if(error->occurred)
			return NULL;

		if(Object_IsInt(rop))
		{
			// float + int
			long long int raw_rop = Object_ToInt(rop, error);

			if(error->occurred)
				return NULL;

			APPLY(raw_lop, (double) raw_rop, res, id)
		}
		else if(Object_IsFloat(rop))
		{
			// float + float
			double raw_rop = Object_ToFloat(rop, error);

			if(error->occurred)
				return NULL;

			APPLY(raw_lop, raw_rop, res, id)
		}
		else
		{
			Error_Report(error, 0, "Relational operation on a non-numeric object");
			return NULL;
		}
	}
	else
	{
		Error_Report(error, 0, "Relational operation on a non-numeric object");
		return NULL;
	}

	#undef APPLY

	return Object_FromBool(res, heap, error);
}

static _Bool step(Runtime *runtime, Error *error)
{
	ASSERT(runtime != NULL);
	ASSERT(error->occurred == 0);
	Opcode opcode;	
	Operand ops[3];
	int     opc = sizeof(ops) / sizeof(ops[0]);

	if(!Executable_Fetch(runtime->frame->exe, runtime->frame->index, &opcode, ops, &opc))
	{
		Error_Report(error, 1, "Invalid instruction index");
		return 0;
	}
	
	runtime->frame->index += 1;

	switch(opcode)
	{
		case OPCODE_NOPE:
		// Do nothing.
		return 1;

		case OPCODE_POS:
		{
			ASSERT(opc == 0);

			if(runtime->frame->used == 0)
			{
				Error_Report(error, 1, "Frame doesn't have enough items on the stack to execute POS");
				return 0;
			}

			/* Do nothing */
			return 1;
		}

		case OPCODE_NEG:
		{
			ASSERT(opc == 0);

			if(runtime->frame->used == 0)
			{
				Error_Report(error, 1, "Frame doesn't have enough items on the stack to execute NEG");
				return 0;
			}

			Object *top = Stack_Top(runtime->stack, 0);
			ASSERT(top != NULL);

			if(!Runtime_Pop(runtime, error, 1))
				return 0;

			Heap *heap = Runtime_GetHeap(runtime);
			ASSERT(heap != NULL);

			if(Object_IsInt(top))
			{
				long long n = Object_ToInt(top, error);

				if(error->occurred)
					return 0;

				top = Object_FromInt(-n, heap, error);
			}
			else if(Object_IsFloat(top))
			{
				double f = Object_ToFloat(top, error);

				if(error->occurred)
					return 0;

				top = Object_FromFloat(-f, heap, error);
			}
			else
			{
				Error_Report(error, 0, "Negation operand on a non-numeric object");
				return 0;
			}

			if(top == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, top))
				return 0;
			return 1;
		}

		case OPCODE_NOT:
		{
			ASSERT(opc == 0);

			if(runtime->frame->used == 0)
			{
				Error_Report(error, 1, "Frame doesn't have enough items on the stack to execute NOT");
				return 0;
			}

			Object *top = Stack_Top(runtime->stack, 0);

			if(!Runtime_Pop(runtime, error, 1))
				return 0;

			ASSERT(top != NULL);

			_Bool v = Object_ToBool(top, error);

			if(error->occurred)
				return 0;

			Object *negated = Object_FromBool(!v, runtime->heap, error);

			if(negated == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, negated))
				return 0;
			return 1;
		}

		case OPCODE_ADD:
		case OPCODE_SUB:
		case OPCODE_MUL:
		case OPCODE_DIV:
		{
			ASSERT(opc == 0);

			Object *rop = Stack_Top(runtime->stack,  0);
			Object *lop = Stack_Top(runtime->stack, -1);

			if(!Runtime_Pop(runtime, error, 2))
				return 0;

			// We managed to pop rop and lop,
			// so we know they're not NULL.
			ASSERT(rop != NULL);
			ASSERT(lop != NULL);

			Object *res = do_math_op(lop, rop, opcode, runtime->heap, error);

			if(res == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, res))
				return 0;
			return 1;
		}

		case OPCODE_EQL:
		case OPCODE_NQL:
		{
			ASSERT(opc == 0);

			Object *rop = Stack_Top(runtime->stack,  0);
			Object *lop = Stack_Top(runtime->stack, -1);

			if(!Runtime_Pop(runtime, error, 2))
				return 0;

			// We managed to pop rop and lop,
			// so we know they're not NULL.
			ASSERT(rop != NULL);
			ASSERT(lop != NULL);

			_Bool rawres = Object_Compare(lop, rop, error);

			if(error->occurred == 1)
				return 0;

			if(opcode == OPCODE_NQL)
				rawres = !rawres;

			Object *res = Object_FromBool(rawres, runtime->heap, error);

			if(res == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, res))
				return 0;
			return 1;
		}

		case OPCODE_LSS:
		case OPCODE_GRT:
		case OPCODE_LEQ:
		case OPCODE_GEQ:
		{
			ASSERT(opc == 0);

			Object *rop = Stack_Top(runtime->stack,  0);
			Object *lop = Stack_Top(runtime->stack, -1);

			if(!Runtime_Pop(runtime, error, 2))
				return 0;

			// We managed to pop rop and lop,
			// so we know they're not NULL.
			ASSERT(rop != NULL);
			ASSERT(lop != NULL);

			Object *res = do_relational_op(lop, rop, opcode, runtime->heap, error);

			if(res == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, res))
				return 0;
			return 1;
		}

		case OPCODE_AND:
		case OPCODE_OR:
		{
			ASSERT(opc == 0);

			Object *rop = Stack_Top(runtime->stack,  0);
			Object *lop = Stack_Top(runtime->stack, -1);

			if(!Runtime_Pop(runtime, error, 2))
				return 0;

			// We managed to pop rop and lop,
			// so we know they're not NULL.
			ASSERT(rop != NULL);
			ASSERT(lop != NULL);

			_Bool raw_rop, raw_lop, raw_res;
			raw_lop = Object_ToBool(lop, error);
			raw_rop = Object_ToBool(rop, error);
			if(error->occurred) return 0;

			switch(opcode)
			{
				case OPCODE_AND: raw_res = raw_lop && raw_rop; break;
				case OPCODE_OR:  raw_res = raw_lop || raw_rop; break;
				default: 
				UNREACHABLE; 
				raw_res = 0;
				break;
			}

			Object *res = Object_FromBool(raw_res, runtime->heap, error);

			if(res == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, res))
				return 0;
			return 1;
		}

		case OPCODE_ASS:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_STRING);

			if(runtime->frame->used == 0)
			{
				Error_Report(error, 0, "Frame has not enough values on the stack");
				return 0;
			}

			Object *val = Stack_Top(runtime->stack, 0);
			ASSERT(val != NULL);

			Object *key = Object_FromString(ops[0].as_string, -1, runtime->heap, error);

			if(key == NULL)
				return 0;

			if(!Object_Insert(runtime->frame->locals, key, val, runtime->heap, error))
				return 0;
			return 1;
		}

		case OPCODE_POP:
		{
			ASSERT(opc == 1);

			if(!Runtime_Pop(runtime, error, ops[0].as_int))
				return 0;
			return 1;
		}

		case OPCODE_CALL:
		{
			ASSERT(opc == 2);
			ASSERT(ops[0].type == OPTP_INT);
			ASSERT(ops[1].type == OPTP_INT);

			int argc = ops[0].as_int;
			int retc = ops[1].as_int;
			ASSERT(argc >= 0 && retc > 0);

			if(runtime->frame->used < argc + 1)
			{
				Error_Report(error, 1, "Frame doesn't own enough objects to execute call");
				return 0;
			}

			Object *callable = Stack_Top(runtime->stack, 0);
			ASSERT(callable != NULL);

			Object *argv[8];

			int max_argc = sizeof(argv) / sizeof(argv[0]);
			if(argc > max_argc)
			{
				Error_Report(error, 1, "Static buffer only allows function calls with up to %d arguments", max_argc);
				return 0;
			}

			for(int i = 0; i < argc; i += 1)
			{
				argv[i] = Stack_Top(runtime->stack, -(i+1));
				ASSERT(argv[i] != NULL);
			}

			ASSERT(error->occurred == 0);
			(void) Runtime_Pop(runtime, error, argc+1);
			ASSERT(error->occurred == 0);

			Object *rets[8];
			unsigned int maxrets = sizeof(rets)/sizeof(rets[0]);

			int num_rets = Object_Call(callable, argv, argc, rets, maxrets, runtime->heap, error);

			if(num_rets < 0)
				return 0;

			// NOTE: Every local object reference is invalidated from here.

			ASSERT(error->occurred == 0);

			for(int g = 0; g < MIN(num_rets, retc); g += 1)
				if(!Runtime_Push(runtime, error, rets[g]))
					return 0;

			for(int g = 0; g < retc - num_rets; g += 1)
			{
				Object *temp = Object_NewNone(Runtime_GetHeap(runtime), error);

				if(temp == NULL)
					return NULL;

				if(!Runtime_Push(runtime, error, temp))
					return 0;
			}
			return 1;
		}

		case OPCODE_SELECT:
		{
			ASSERT(opc == 0);

			if(runtime->frame->used < 2)
			{
				Error_Report(error, 1, "Frame has not enough values on the stack to run SELECT instruction");
				return 0;
			}

			Object *col = Stack_Top(runtime->stack, -1);
			Object *key = Stack_Top(runtime->stack,  0);

			ASSERT(col != NULL && key != NULL);

			if(!Runtime_Pop(runtime, error, 2))
				return 0;

			ASSERT(error->occurred == 0);

			Error dummy;
			Error_Init(&dummy); // We want to catch the error reported by this Object_Select.

			Object *val = Object_Select(col, key, runtime->heap, &dummy);

			if(val == NULL)
				{
					Error_Free(&dummy);

					val = Object_NewNone(runtime->heap, error);

					if(val == NULL)
						return 0;
				}

			ASSERT(error->occurred == 0);

			if(!Runtime_Push(runtime, error, val))
				return 0;

			ASSERT(error->occurred == 0);
			return 1;
		}

		case OPCODE_INSERT:
		{
			ASSERT(opc == 0);

			if(runtime->frame->used < 3)
			{
				Error_Report(error, 1, "Frame has not enough values on the stack to run INSERT instruction");
				return 0;
			}

			Object *col = Stack_Top(runtime->stack, -2);
			Object *key = Stack_Top(runtime->stack, -1);
			Object *val = Stack_Top(runtime->stack,  0);

			ASSERT(col != NULL && key != NULL && val != NULL);

			if(!Runtime_Pop(runtime, error, 2))
				return 0;

			if(!Object_Insert(col, key, val, runtime->heap, error))
				return 0;
			return 1;
		}

		case OPCODE_INSERT2:
		{
			ASSERT(opc == 0);

			if(runtime->frame->used < 3)
			{
				Error_Report(error, 1, "Frame has not enough values on the stack to run INSERT2 instruction");
				return 0;
			}

			Object *val = Stack_Top(runtime->stack, -2);
			Object *col = Stack_Top(runtime->stack, -1);
			Object *key = Stack_Top(runtime->stack,  0);

			ASSERT(col != NULL && key != NULL && val != NULL);

			if(!Runtime_Pop(runtime, error, 2))
				return 0;

			if(!Object_Insert(col, key, val, runtime->heap, error))
				return 0;
			return 1;
		}

		case OPCODE_PUSHINT:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_INT);

			Object *obj = Object_FromInt(ops[0].as_int, runtime->heap, error);

			if(obj == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHFLT:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_FLOAT);

			Object *obj = Object_FromFloat(ops[0].as_float, runtime->heap, error);

			if(obj == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHSTR:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_STRING);

			Object *obj = Object_FromString(ops[0].as_string, -1, runtime->heap, error);

			if(obj == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHVAR:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_STRING);

			Object *key = Object_FromString(ops[0].as_string, -1, runtime->heap, error);
				
			if(key == NULL)
				return 0;

			Object *locations[] = {
				runtime->frame->locals,
				runtime->frame->closure,
				Runtime_GetBuiltins(runtime),
			};
				
			Object *obj = NULL;

			for(int p = 0; obj == NULL && (unsigned int) p < sizeof(locations)/sizeof(locations[0]); p += 1)
			{
				if(locations[p] == NULL)
					continue;

				obj = Object_Select(locations[p], key, Runtime_GetHeap(runtime), error);
			}

			if(obj == NULL)
			{
				if(error->occurred == 0)
					// There's no such variable.
					Error_Report(error, 0, "Reference to undefined variable \"%s\"", ops[0].as_string);
				return 0;
			}

			if(!Runtime_Push(runtime, error, obj))
				return 0;

			return 1;
		}

		case OPCODE_PUSHNNE:
		{
			ASSERT(opc == 0);

			Object *obj = Object_NewNone(runtime->heap, error);

			if(obj == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHTRU:
		{
			ASSERT(opc == 0);

			Object *obj = Object_FromBool(1, runtime->heap, error);

			if(obj == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHFLS:
		{
			ASSERT(opc == 0);

			Object *obj = Object_FromBool(0, runtime->heap, error);

			if(obj == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHFUN:
		{
			ASSERT(opc == 2);
			ASSERT(ops[0].type == OPTP_IDX);
			ASSERT(ops[1].type == OPTP_INT);

			Object *closure = Object_NewClosure(runtime->frame->closure, runtime->frame->locals, Runtime_GetHeap(runtime), error);

			if(closure == NULL)
				return 0;

			Object *obj = Object_FromNojaFunction(runtime, runtime->frame->exe, ops[0].as_int, ops[1].as_int, closure, runtime->heap, error);

			if(obj == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHLST:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_INT);

			Object *obj = Object_NewList(ops[0].as_int, runtime->heap, error);

			if(obj == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHMAP:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_INT);

			Object *obj = Object_NewMap(ops[0].as_int, runtime->heap, error);

			if(obj == NULL)
				return 0;

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHTYPTYP:
		{
			ASSERT(opc == 0);

			Object *obj = (Object*) Object_GetTypeType();
			ASSERT(obj != NULL);

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHNNETYP:
		{
			ASSERT(opc == 0);

			Object *obj = (Object*) Object_GetNoneType();
			ASSERT(obj != NULL);

			if(!Runtime_Push(runtime, error, obj))
				return 0;
			return 1;
		}

		case OPCODE_PUSHTYP:
		{
			ASSERT(opc == 0);

			if(runtime->frame->used < 1)
			{
				Error_Report(error, 1, "Frame has not enough values on the stack to run PUSHTYP instruction");
				return 0;
			}

			Object *top = Stack_Top(runtime->stack, 0);
			ASSERT(top != NULL);

			Object *typ = (Object*) Object_GetType(top);
			ASSERT(typ != NULL);
			
			if(!Runtime_Push(runtime, error, typ))
				return 0;
			return 1;
		}

		case OPCODE_EXIT:
		{
			ASSERT(opc == 0);

			Object *vars = runtime->frame->locals;
			ASSERT(vars != NULL);

			if(!Runtime_Push(runtime, error, vars))
				return 0;
			return 0;
		}

		case OPCODE_RETURN:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_INT);
			int retc = ops[0].as_int;
			UNUSED(retc);
			ASSERT(retc >= 0);
			ASSERT(retc == runtime->frame->used);
			return 0;
		}

		case OPCODE_ERROR:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_STRING);
			const char *msg = ops[0].as_string;
			Error_Report(error, 0, "%s", msg);
			return 0;
		}

		case OPCODE_JUMP:
		ASSERT(opc == 1);
		ASSERT(ops[0].type == OPTP_IDX);
		runtime->frame->index = ops[0].as_int;
		return 1;

		case OPCODE_JUMPIFANDPOP:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_IDX);
				
			long long int target = ops[0].as_int;

			if(runtime->frame->used == 0)
			{
				Error_Report(error, 1, "Frame doesn't have enough items on the stack to execute JUMPIFNOTANDPOP");
				return 0;
			}

			Object *top = Stack_Top(runtime->stack, 0);

			if(!Runtime_Pop(runtime, error, 1))
				return 0;			

			ASSERT(top != NULL);

			if(!Object_IsBool(top))
			{
				Error_Report(error, 0, "Not a boolean");
				return 0;
			}

			if(Object_ToBool(top, error)) // This can't fail because we know it's a bool.
				runtime->frame->index = target;

			return 1;
		}

		case OPCODE_JUMPIFNOTANDPOP:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_IDX);
				
			long long int target = ops[0].as_int;

			if(runtime->frame->used == 0)
			{
				Error_Report(error, 1, "Frame doesn't have enough items on the stack to execute JUMPIFNOTANDPOP");
				return 0;
			}

			Object *top = Stack_Top(runtime->stack, 0);

			if(!Runtime_Pop(runtime, error, 1))
				return 0;			

			ASSERT(top != NULL);

			if(!Object_IsBool(top))
			{
				Error_Report(error, 0, "Not a boolean");
				return 0;
			}

			if(!Object_ToBool(top, error)) // This can't fail because we know it's a bool.
				runtime->frame->index = target;

			return 1;
		}

		default:
		UNREACHABLE;
		return 0;
	}

	return 1;
}

static _Bool collect(Runtime *runtime, Error *error)
{
	Frame *frame = runtime->frame;

	if(!Heap_StartCollection(runtime->heap, error))
		return 0;

	Heap_CollectReference(&runtime->builtins,  runtime->heap);

	while(frame)
	{
		Heap_CollectReference(&frame->locals,  runtime->heap);
		Heap_CollectReference(&frame->closure, runtime->heap);
		frame = frame->prev;
	}

	for(unsigned int i = 0; i < Stack_Size(runtime->stack); i += 1)
	{
		Object **ref = (Object**) Stack_TopRef(runtime->stack, -i);
		ASSERT(ref != NULL);

		Heap_CollectReference(ref, runtime->heap);
	}

	return Heap_StopCollection(runtime->heap);
}

int run(Runtime *runtime, Error *error, 
	    Executable *exe, int index, 
	    Object *closure, 
	    Object **argv, int argc, 
	    Object **rets, int maxretc)
{
	ASSERT(runtime != NULL);
	ASSERT(error != NULL);
	ASSERT(exe != NULL);
	ASSERT(index >= 0);
	ASSERT(argc >= 0);

	if(runtime->depth == MAX_FRAMES)
	{
		Error_Report(error, 1, "Maximum nested call limit of %d was reached", MAX_FRAMES);
		return -1;
	}

	ASSERT(runtime->depth < MAX_FRAMES);
		
	// Initialize the frame.
	Frame frame;
	{
		frame.prev = NULL;
		frame.closure = closure;
		frame.locals = Object_NewMap(-1, runtime->heap, error);
		frame.exe  = Executable_Copy(exe);
		frame.index = index;
		frame.used  = 0;

		if(frame.locals == NULL)
			return -1;

		if(frame.exe == NULL)
		{
			Error_Report(error, 1, "Failed to copy executable");
			return -1;
		}
	
		// Add the frame to the runtime.
		frame.prev = runtime->frame;
		runtime->frame = &frame;
		runtime->depth += 1;
	}

	// This is what the function will return.
	int retc = -1;

	// Push the initial values of the frame.
	for(int i = 0; i < argc; i += 1)
		if(!Runtime_Push(runtime, error, argv[i]))
			goto cleanup;

	// Run the code.

	if(runtime->callback_addr != NULL)
	{
		if(!runtime->callback_addr(runtime, runtime->callback_userp))
			Error_Report(error, 0, "Forced abortion");
		else
			while(step(runtime, error))
			{
				if(!runtime->callback_addr(runtime, runtime->callback_userp))
				{
					Error_Report(error, 0, "Forced abortion");
					break;
				}

				//printf("%2.2f%% percent.\n", Heap_GetUsagePercentage(runtime->heap));

				if(Heap_GetUsagePercentage(runtime->heap) > 100)
					if(!collect(runtime, error))
						break;
			}
	}
	else
		while(step(runtime, error))
		{
			if(Heap_GetUsagePercentage(runtime->heap) > 100)
				if(!collect(runtime, error))
					break;

			//printf("%2.2f%% percent.\n", Heap_GetUsagePercentage(runtime->heap));
		}

	// If an error occurred, we want to return NULL.
	if(error->occurred == 0)
	{
		retc = MIN(frame.used, maxretc);

		for(int i = 0; i < retc; i += 1)
		{
			rets[i] = Stack_Top(runtime->stack, i - retc + 1);
			ASSERT(rets[i] != NULL);
		}
	}

cleanup:
	// Remove the frame-owned items from the stack.
	// This can't fail.
	(void) Stack_Pop(runtime->stack, frame.used);

	// Deinitialize the frame.
	{
	 	// Remove the frame from the runtime.
		runtime->frame = runtime->frame->prev;
		runtime->depth -= 1;

		// Deallocate the fields.
		Executable_Free(frame.exe);
	}

	return retc;
}