#include <stdlib.h>
#include "../utils/defs.h"
#include "../utils/stack.h"
#include "runtime.h"

#define MAX_FRAME_STACK 16
#define MAX_FRAMES 16

typedef struct Frame Frame;

struct Frame {
	Frame  *prev;
	Object *vars;
	Executable *exe;
	int index, used;
};

struct xRuntime {
	void *callback_userp;
	_Bool (*callback_addr)(Runtime*, void*);
	Object *builtins;
	int    depth;
	Frame *frame;
	Stack *stack;
	Heap  *heap;
};

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
		return NULL;
	else
		return runtime->frame->exe;
}

Runtime *Runtime_New(int stack_size, int heap_size, void *callback_userp, _Bool (*callback_addr)(Runtime*, void*))
{
	if(stack_size < 0)
		stack_size = 1024;

	if(heap_size < 0)
		heap_size = 65536;

	Runtime *runtime;

	{
		runtime = malloc(sizeof(Runtime));

		if(runtime == NULL)
			return NULL;

		runtime->heap = Heap_New(heap_size);

		if(runtime->heap == NULL)
			{
				free(runtime);
				return NULL;
			}

		runtime->stack = Stack_New(stack_size);

		if(runtime->stack == NULL)
			{
				Heap_Free(runtime->heap);
				free(runtime);
			}

		runtime->callback_userp = callback_userp;
		runtime->callback_addr = callback_addr;
		runtime->builtins = NULL;
		runtime->frame = NULL;
		runtime->depth = 0;
		
	}

	return runtime;
}

void Runtime_Free(Runtime *runtime)
{
	Heap_Free(runtime->heap);
	Stack_Free(runtime->stack);
	free(runtime);
}

Object *Runtime_GetBuiltins(Runtime *runtime, Error *error)
{
	if(runtime->builtins == NULL)
		runtime->builtins = Object_NewMap(-1, runtime->heap, error);

	return runtime->builtins;
}

_Bool Runtime_Push(Runtime *runtime, Error *error, Object *obj)
{
	assert(runtime != NULL);
	assert(error != NULL);
	assert(obj != NULL);

	if(runtime->depth == 0)
		{
			Error_Report(error, 0, "There are no frames on the stack");
			return 0;
		}

	assert(runtime->frame->used <= MAX_FRAME_STACK);
	
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
	assert(runtime != NULL);
	assert(error != NULL);

	if(runtime->depth == 0)
		{
			Error_Report(error, 0, "There are no frames on the stack");
			return 0;
		}

	assert(runtime->frame->used >= 0);

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

	assert(runtime->frame->used >= 0);
	
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
	assert(runtime->depth >= 0);

	Snapshot *snapshot = malloc(sizeof(Snapshot) + sizeof(SnapshotNode) * runtime->depth);

	if(snapshot == NULL)
		return NULL;

	{
		Frame *f = runtime->frame;

		snapshot->depth = 0;

		while(snapshot->depth < runtime->depth)
			{
				assert(f != NULL);

				SnapshotNode *node = snapshot->nodes + snapshot->depth;

				node->exe   = Executable_Copy(f->exe);
				node->index = f->index;

				if(node->exe == NULL)
					goto abort;

				f = f->prev;
				snapshot->depth += 1;
			}

		assert(f == NULL);
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
	assert(snapshot != NULL);
	assert(fp != NULL);

	fprintf(fp, "  (Snapshot can't be printed yet)\n");
}

static Object *do_math_op(Object *lop, Object *rop, Opcode opcode, Heap *heap, Error *error)
{
	assert(lop != NULL);
	assert(rop != NULL);

	#define APPLY(x, y, z, id) 								\
		switch(opcode)										\
			{												\
				case OPCODE_ADD: (z) = (x) + (y); break;	\
				case OPCODE_SUB: (z) = (x) - (y); break;	\
				case OPCODE_MUL: (z) = (x) * (y); break;	\
				case OPCODE_DIV: (z) = (x) / (y); break;	\
				default: assert(0); break;					\
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

					long long int raw_res;

					APPLY(raw_lop, raw_rop, raw_res, id)

					res = Object_FromInt(raw_res, heap, error);
				}
			else if(Object_IsFloat(rop))
				{
					// int + float
					double raw_rop = Object_ToFloat(rop, error);

					if(error->occurred)
						return NULL;

					double raw_res;

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

					double raw_res;

					APPLY(raw_lop, (double) raw_rop, raw_res, id)

					res = Object_FromFloat(raw_res, heap, error);
				}
			else if(Object_IsFloat(rop))
				{
					// float + float
					double raw_rop = Object_ToFloat(rop, error);

					if(error->occurred)
						return NULL;

					double raw_res;

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

static _Bool step(Runtime *runtime, Error *error)
{
	assert(runtime != NULL);

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
			Error_Report(error, 1, "POS not implemented");
			return 0;

			case OPCODE_NEG:
			Error_Report(error, 1, "NEG not implemented");
			return 0;

			case OPCODE_ADD:
			case OPCODE_SUB:
			case OPCODE_MUL:
			case OPCODE_DIV:
			{
				assert(opc == 0);

				Object *rop = Stack_Top(runtime->stack,  0);
				Object *lop = Stack_Top(runtime->stack, -1);

				if(!Runtime_Pop(runtime, error, 2))
					return 0;

				// We managed to pop rop and lop,
				// so we know they're not NULL.
				assert(rop != NULL);
				assert(lop != NULL);

				Object *res = do_math_op(lop, rop, opcode, runtime->heap, error);

				if(res == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, res))
					return 0;
				break;
			}

			case OPCODE_ASS:
			{
				assert(opc == 1);
				assert(ops[0].type == OPTP_STRING);

				if(runtime->frame->used == 0)
					{
						Error_Report(error, 0, "Frame has not enough values on the stack");
						return 0;
					}

				Object *val = Stack_Top(runtime->stack, 0);
				assert(val != NULL);

				Object *key = Object_FromString(ops[0].as_string, -1, runtime->heap, error);
								
				if(key == NULL)
					return 0;
				
				if(!Object_Insert(runtime->frame->vars, key, val, runtime->heap, error))
					return 0;
				return 1;
			}

			case OPCODE_POP:
			{
				assert(opc == 1);

				if(!Runtime_Pop(runtime, error, ops[0].as_int))
					return 0;
				return 1;
			}

			case OPCODE_CALL:
			{
				assert(opc == 1);
				assert(ops[0].type == OPTP_INT);

				int argc = ops[0].as_int;
				assert(argc >= 0);

				if(runtime->frame->used < argc + 1)
					{
						Error_Report(error, 1, "Frame doesn't own enough objects to execute call");
						return 0;
					}

				Object *callable = Stack_Top(runtime->stack, 0);
				assert(callable != NULL);

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
						assert(argv[i] != NULL);
					}

				(void) Runtime_Pop(runtime, error, argc+1);
				assert(error->occurred == 0);

				Object *obj = Object_Call(callable, argv, argc, runtime->heap, error);

				if(obj == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, obj))
					return 0;
				return 1;
			}

			case OPCODE_PUSHINT:
			{
				assert(opc == 1);
				assert(ops[0].type == OPTP_INT);

				Object *obj = Object_FromInt(ops[0].as_int, runtime->heap, error);

				if(obj == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, obj))
					return 0;
				return 1;
			}

			case OPCODE_PUSHFLT:
			{
				assert(opc == 1);
				assert(ops[0].type == OPTP_FLOAT);

				Object *obj = Object_FromFloat(ops[0].as_float, runtime->heap, error);

				if(obj == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, obj))
					return 0;
				return 1;
			}

			case OPCODE_PUSHSTR:
			{
				assert(opc == 1);
				assert(ops[0].type == OPTP_STRING);

				Object *obj = Object_FromString(ops[0].as_string, -1, runtime->heap, error);

				if(obj == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, obj))
					return 0;
				return 1;
			}

			case OPCODE_PUSHVAR:
			{
				assert(opc == 1);
				assert(ops[0].type == OPTP_STRING);

				Object *key = Object_FromString(ops[0].as_string, -1, runtime->heap, error);
				
				if(key == NULL)
					return 0;

				Object *obj = Object_Select(runtime->frame->vars, key, runtime->heap, error);

				if(obj == NULL && error->occurred == 0)
					{
						// Variable not defined locally.

						if(runtime->builtins != NULL)
							obj = Object_Select(runtime->builtins, key, runtime->heap, error);

						if(obj == NULL)
							{
								if(error->occurred == 0)
									// There's no such variable.
									Error_Report(error, 1, "Reference to undefined variable \"%s\"", ops[0].as_string);
								return 0;
							}
					}
				else if(obj == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, obj))
					return 0;

				return 1;
			}

			case OPCODE_PUSHNNE:
			{
				assert(opc == 0);

				Object *obj = Object_NewNone(runtime->heap, error);

				if(obj == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, obj))
					return 0;
				return 1;
			}

			case OPCODE_PUSHTRU:
			{
				assert(opc == 0);

				Object *obj = Object_FromBool(1, runtime->heap, error);

				if(obj == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, obj))
					return 0;
				return 1;
			}

			case OPCODE_PUSHFLS:
			{
				assert(opc == 0);

				Object *obj = Object_FromBool(0, runtime->heap, error);

				if(obj == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, obj))
					return 0;
				return 1;
			}

			case OPCODE_PUSHFUN:
			{
				assert(opc == 2);
				assert(ops[0].type == OPTP_INT);
				assert(ops[1].type == OPTP_INT);

				Object *obj = Object_FromNojaFunction(runtime, runtime->frame->exe, ops[0].as_int, ops[1].as_int, runtime->heap, error);

				if(obj == NULL)
					return 0;

				if(!Runtime_Push(runtime, error, obj))
					return 0;
				return 1;
			}

			case OPCODE_RETURN:
			return 0;

			case OPCODE_JUMP:
			assert(opc == 1);
			assert(ops[0].type == OPTP_INT);
			runtime->frame->index = ops[0].as_int;
			return 1;

			case OPCODE_JUMPIFANDPOP:
			{
				assert(opc == 1);
				assert(ops[0].type == OPTP_INT);
				
				long long int target = ops[0].as_int;

				if(runtime->frame->used == 0)
					{
						Error_Report(error, 1, "Frame doesn't have enough items on the stack to execute JUMPIFNOTANDPOP");
						return 0;
					}

				Object *top = Stack_Top(runtime->stack, 0);

				if(!Runtime_Pop(runtime, error, 1))
					return 0;			

				assert(top != NULL);

				if(!Object_IsBool(top))
					{
						Error_Report(error, 1, "JUMPIFNOTANDPOP expected a boolean on the stack");
						return 0;
					}

				if(Object_ToBool(top, error)) // This can't fail because we know it's a bool.
					runtime->frame->index = target;

				return 1;
			}

			case OPCODE_JUMPIFNOTANDPOP:
			{
				assert(opc == 1);
				assert(ops[0].type == OPTP_INT);
				
				long long int target = ops[0].as_int;

				if(runtime->frame->used == 0)
					{
						Error_Report(error, 1, "Frame doesn't have enough items on the stack to execute JUMPIFNOTANDPOP");
						return 0;
					}

				Object *top = Stack_Top(runtime->stack, 0);

				if(!Runtime_Pop(runtime, error, 1))
					return 0;			

				assert(top != NULL);

				if(!Object_IsBool(top))
					{
						Error_Report(error, 1, "JUMPIFNOTANDPOP expected a boolean on the stack");
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

Object *run(Runtime *runtime, Error *error, Executable *exe, int index, Object **argv, int argc)
{
	assert(runtime != NULL);
	assert(error != NULL);
	assert(exe != NULL);
	assert(index >= 0);
	assert(argc >= 0);

	if(runtime->depth == MAX_FRAMES)
		{
			Error_Report(error, 1, "Maximum nested call limit of %d was reached", MAX_FRAMES);
			return NULL;
		}

	assert(runtime->depth < MAX_FRAMES);
		
	// Initialize the frame.
	Frame frame;
	{
		frame.prev = NULL;
		frame.vars = Object_NewMap(-1, runtime->heap, error);
		frame.exe  = Executable_Copy(exe);
		frame.index = index;
		frame.used  = 0;

		if(frame.vars == NULL)
			return NULL;

		if(frame.exe == NULL)
			{
				Error_Report(error, 1, "Failed to copy executable");
				return NULL;
			}
	
		// Add the frame to the runtime.
		frame.prev = runtime->frame;
		runtime->frame = &frame;
		runtime->depth += 1;
	}

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
					if(!runtime->callback_addr(runtime, runtime->callback_userp))
						{
							Error_Report(error, 0, "Forced abortion");
							break;
						}
		}
	else
		while(step(runtime, error));

	// This is what the function will return.
	Object *result = NULL;

	// If an error occurred, we want to return NULL.
	if(error->occurred == 0)
		{
			// If the step function left something
			// on the stack, we return that. If it
			// didn't, we return some other default
			// value like "none".
			if(frame.used == 0)
				{
					// Nothing to return on the stack. Set to none.
					result = Object_NewNone(runtime->heap, error);
				}
			else
				{
					result = Stack_Top(runtime->stack, 0);
					assert(result != NULL);
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

	return result;
}