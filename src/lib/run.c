#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "utils/defs.h"
#include "utils/path.h"
#include "compiler/compile.h"
#include "assembler/assemble.h"

static int runExecutableAtIndex(Runtime *runtime, Error *error,
		    				    Executable *exe, int index,
		    				    Object *closure,
		    				    Object *rets[static MAX_RETS],
						        Object *argv[], int argc);

typedef struct {
	Object base;
	const char *name;
	Runtime *runtime;
	Executable *exe;
	int index, argc;
	Object *closure;
	TimingID timing_id;
} FunctionObject;

static _Bool func_free(Object *self, Error *error)
{
	(void) error;
	
	FunctionObject *func = (FunctionObject*) self;
	Executable_Free(func->exe);	
	return 1;
}

static void func_walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp)
{
	FunctionObject *func = (FunctionObject*) self;
	callback(&func->closure, userp);
}

static int func_call(Object *self, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Heap *heap, Error *error)
{
	ASSERT(self != NULL && heap != NULL && error != NULL);
	
	FunctionObject *func = (FunctionObject*) self;

	ASSERT(func->exe != NULL);
	ASSERT(func->argc >= 0);
	ASSERT(func->index >= 0);

	// Make sure the right amount of arguments is provided.

	Object **argv2;

	int expected_argc = func->argc;

	if(expected_argc < (int) argc)
	{
		// Nothing to be done. By using
		// the right argc the additional
		// arguments are ignored implicitly.
		argv2 = argv;
	}
	else if(expected_argc > (int) argc)
	{
		// Some arguments are missing.
		argv2 = malloc(sizeof(Object*) * expected_argc);

		if(argv2 == NULL)
		{
			Error_Report(error, ErrorType_INTERNAL, "No memory");
			return -1;
		}

		// Copy the provided arguments.
		for(int i = 0; i < (int) argc; i += 1)
			argv2[i] = argv[i];

		// Set the unspecified arguments to none.
		for(int i = argc; i < expected_argc; i += 1)
		{
			argv2[i] = Object_NewNone(heap, error);

			if(argv2[i] == NULL)
				return -1;
		}
	}
	else
		// The right amount of arguments was provided.
		argv2 = argv;

	clock_t begin;
	TimingID timing_id;
	TimingTable *timing_table = Runtime_GetTimingTable(func->runtime);
	if (timing_table != NULL) {
		begin = clock();

		// Need to save the object's member
	    // before the run function since it
	    // may trigger a GC cycle invalidating
	    // the object pointer.
		timing_id = func->timing_id;
	}

	int retc = runExecutableAtIndex(func->runtime, error, func->exe, func->index, func->closure, rets, argv2, expected_argc);

	if (timing_table != NULL) {
		double time = (double) (clock() - begin) / CLOCKS_PER_SEC;
		TimingTable_sumCallTime(timing_table, timing_id, time);
	}

	// NOTE: Every object reference is invalidated from here.
	
	if(argv2 != argv)
		free(argv2);

	return retc;
}

static TypeObject t_func = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "function",
	.size = sizeof (FunctionObject),
	.call = func_call,
	.walk = func_walk,
	.free = func_free,
};

/* Symbol: Object_FromNojaFunction
 *
 *   Creates an object from a noja executable structure.
 *
 * Args:
 *   - runtime: The reference to an instanciated Runtime.
 *
 *   - exe: A noja executable.
 *
 *   - index: The index of the first bytecode instruction
 *            of the noja function within the executable.
 *
 *   - argc: The number of arguments the function expects. 
 *           It must be positive (unlike [Object_FromNativeFunction],
 *           where -1 means variadic).
 *
 *   - closure: An object containing variables that will be
 *              accessible from the noja function other than
 *              the ones that will be defined inside it.
 *
 *   - heap: The heap that will be used to allocate the object.
 *           It can't be NULL.
 *
 *   - error: Output parameter where error information is stored.
 *            It can't be NULL.
 *
 * Returns:
 *   The newly created object. If an error occurred, NULL is returned
 *   and information about the error is stored in the [error] argument.
 */
Object *Object_FromNojaFunction(Runtime *runtime, const char *name, Executable *exe, int index, int argc, Object *closure, Heap *heap, Error *error)
{
	ASSERT(runtime != NULL);
	ASSERT(exe != NULL);
	ASSERT(index >= 0);
	ASSERT(argc >= 0);
	ASSERT(heap != NULL);
	ASSERT(error != NULL);

	FunctionObject *func = (FunctionObject*) Heap_Malloc(heap, &t_func, error);

	if(func == NULL)
		return NULL;

	Executable *exe_copy = Executable_Copy(exe);

	if(exe_copy == NULL)
	{
		Error_Report(error, ErrorType_INTERNAL, "Failed to copy executable");
		return NULL;
	}

	func->runtime = runtime;
	func->name = name; // Should this be copied?
	func->exe = exe_copy;
	func->index = index;
	func->argc = argc;
	func->closure = closure;

	TimingTable *table = Runtime_GetTimingTable(runtime);
	if (table != NULL) {
		#warning "TODO: Calculate line number"
		size_t line = 0;
		Source *src = Executable_GetSource(exe);
		func->timing_id = TimingTable_newEntry(table, src, line, name);
	}

	return (Object*) func;
}


typedef struct {
	Object base;
	Runtime *runtime;
	int (*callback)(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[MAX_RETS], Error *error);
	int argc;
} NativeFunctionObject;

static int native_func_call(Object *self, Object **argv, unsigned int argc, Object *rets[static MAX_RETS],  Heap *heap, Error *error)
{
	ASSERT(self != NULL);
	ASSERT(heap != NULL);
	ASSERT(error != NULL);
			
	NativeFunctionObject *func = (NativeFunctionObject*) self;

	// If the function isn't variadic, make sure
	// the right amount of arguments is provided.

	Object **argv2;
	int 	 argc2;

	int expected_argc = func->argc;

	if(expected_argc < 0 || expected_argc == (int) argc)
	{
		// The function is variadic or the right 
		// amount of arguments was provided.
		argv2 = argv;
		argc2 = argc;
	}
	else if(expected_argc < (int) argc)
	{
		// Nothing to be done. By using
		// the right argc the additional
		// arguments are ignored implicitly.
		argv2 = argv;
		argc2 = expected_argc;
	}
	else if(expected_argc > (int) argc)
	{
		// Some arguments are missing.
		argv2 = malloc(sizeof(Object*) * expected_argc);
		argc2 = expected_argc;
			
		if(argv2 == NULL)
		{
			Error_Report(error, 1, "No memory");
			return -1;
		}

		// Copy the provided arguments.
		for(int i = 0; i < (int) argc; i += 1)
			argv2[i] = argv[i];

		// Set the unspecified arguments to none.
		for(int i = argc; i < expected_argc; i += 1)
		{
			argv2[i] = Object_NewNone(heap, error);

			if(argv2[i] == NULL)
			{
				free(argv2);
				return -1;
			}
		}
	} else {
		UNREACHABLE;
		argv2 = NULL;
		argc2 = -1;
	}

	if (!Runtime_PushNativeFrame(func->runtime, error))
    	return -1;

	ASSERT(func->callback != NULL);
	int retc = func->callback(func->runtime, argv2, argc2, rets, error);
		
	// NOTE: Since the callback may have executed some bytecode, a GC
	//       cycle may have been triggered, therefore we must assume
	//       every object reference that was locally saved is invalidated 
	//       from here (the returned object is good tho).

	if(argv2 != argv)
		free(argv2);

	if (retc >= 0 && !Runtime_PopFrame(func->runtime))
    	return -1;

	return retc;
}

static TypeObject t_nfunc = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "native function",
	.size = sizeof (NativeFunctionObject),
	.call = native_func_call,	
};

/* Symbol: Object_FromNativeFunction
 *
 *   Creates an object from a function pointer.
 *
 * Args:
 *   - runtime: The reference to an instanciated Runtime. This must be
 *              provided so that the callback can also access it.
 *
 *   - callback: The native function to be executed when this object
 *               is called.
 *
 *   - argc: The number of arguments the function expects. If -1 is
 *           provided, then the function is considered to be variadic.
 *
 *   - heap: The heap that will be used to allocate the object.
 *           It can't be NULL.
 *
 *   - error: Output parameter where error information is stored.
 *            It can't be NULL.
 *
 * Returns:
 *   The newly created object. If an error occurred, NULL is returned
 *   and information about the error is stored in the [error] argument.
 */
Object *Object_FromNativeFunction(Runtime *runtime, int (*callback)(Runtime*, Object**, unsigned int, Object*[static MAX_RETS], Error*), int argc, Heap *heap, Error *error)
{
	ASSERT(callback != NULL);

	NativeFunctionObject *func = (NativeFunctionObject*) Heap_Malloc(heap, &t_nfunc, error);

	if(func == NULL)
		return NULL;

	func->runtime = runtime;
	func->callback = callback;
	func->argc = argc;

	return (Object*) func;
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
				Error_Report(error, ErrorType_RUNTIME, "Division by zero"); \
				return NULL; 							\
			} 											\
			(z) = (x) / (y); 							\
			break;										\
			default: UNREACHABLE; break;				\
		}

	Object *res;

	if(Object_IsInt(lop))
	{
		long long int raw_lop = Object_GetInt(lop);

		if(Object_IsInt(rop))
		{
			// int + int
			long long int raw_rop = Object_GetInt(rop);
			long long int raw_res = 0;
			APPLY(raw_lop, raw_rop, raw_res, id)
			res = Object_FromInt(raw_res, heap, error);
		}
		else if(Object_IsFloat(rop))
		{
			// int + float
			double raw_rop = Object_GetFloat(rop);
			double raw_res = 0;
			APPLY((double) raw_lop, raw_rop, raw_res, id)
			res = Object_FromFloat(raw_res, heap, error);
		}
		else
		{
			Error_Report(error, ErrorType_RUNTIME, "Arithmetic operation on a non-numeric object");
			return NULL;
		}
	}
	else if(Object_IsFloat(lop))
	{
		double raw_lop = Object_GetFloat(lop);

		if(Object_IsInt(rop))
		{
			// float + int
			long long int raw_rop = Object_GetInt(rop);
			double raw_res = 0;
			APPLY(raw_lop, (double) raw_rop, raw_res, id)
			res = Object_FromFloat(raw_res, heap, error);
		}
		else if(Object_IsFloat(rop))
		{
			// float + float
			double raw_rop = Object_GetFloat(rop);
			double raw_res = 0;
			APPLY(raw_lop, raw_rop, raw_res, id)
			res = Object_FromFloat(raw_res, heap, error);
		}
		else
		{
			Error_Report(error, ErrorType_RUNTIME, "Arithmetic operation on a non-numeric object");
			return NULL;
		}
	}
	else
	{
		Error_Report(error, ErrorType_RUNTIME, "Arithmetic operation on a non-numeric object");
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
		long long int raw_lop = Object_GetInt(lop);
		if(Object_IsInt(rop))
		{
			// int + int
			long long int raw_rop = Object_GetInt(rop);
			APPLY(raw_lop, raw_rop, res, id)
		}
		else if(Object_IsFloat(rop))
		{
			// int + float
			double raw_rop = Object_GetFloat(rop);
			APPLY((double) raw_lop, raw_rop, res, id)
		}
		else
		{
			Error_Report(error, ErrorType_RUNTIME, "Relational operation on a non-numeric object");
			return NULL;
		}
	}
	else if(Object_IsFloat(lop))
	{
		double raw_lop = Object_GetFloat(lop);
		if(Object_IsInt(rop))
		{
			// float + int
			long long int raw_rop = Object_GetInt(rop);
			APPLY(raw_lop, (double) raw_rop, res, id)
		}
		else if(Object_IsFloat(rop))
		{
			// float + float
			double raw_rop = Object_GetFloat(rop);
			APPLY(raw_lop, raw_rop, res, id)
		}
		else
		{
			Error_Report(error, ErrorType_RUNTIME, "Relational operation on a non-numeric object");
			return NULL;
		}
	}
	else
	{
		Error_Report(error, ErrorType_RUNTIME, "Relational operation on a non-numeric object");
		return NULL;
	}

	#undef APPLY

	return Object_FromBool(res, heap, error);
}

static _Bool runInstruction(Runtime *runtime, Error *error)
{
	ASSERT(runtime != NULL);
	ASSERT(error->occurred == 0);

	Heap *heap = Runtime_GetHeap(runtime);
	Executable *exe = Runtime_GetCurrentExecutable(runtime);
	ASSERT(exe != NULL);
	int index = Runtime_GetCurrentIndex(runtime);
	Opcode opcode;
	Operand ops[3];
	int     opc = sizeof(ops) / sizeof(ops[0]);
	
	if(!Executable_Fetch(exe, index, &opcode, ops, &opc))
	{
		Error_Report(error, ErrorType_INTERNAL, "Invalid instruction index %d", index);
		return 0;
	}
	
	Runtime_SetInstructionIndex(runtime, index+1);

	switch(opcode)
	{
		case OPCODE_NOPE:
		// Do nothing.
		return 1;

		case OPCODE_POS:
		{
			ASSERT(opc == 0);

			if(Runtime_Top(runtime, 0) == NULL)
			{
				Error_Report(error, ErrorType_INTERNAL, "Frame doesn't have enough items on the stack to execute POS");
				return 0;
			}

			/* Do nothing */
			return 1;
		}

		case OPCODE_NEG:
		{
			ASSERT(opc == 0);
			Object *top;
			if(!Runtime_Pop(runtime, error, &top, 1))
				return 0;

			if(Object_IsInt(top))
			{
				long long n = Object_GetInt(top);
				top = Object_FromInt(-n, heap, error);
			}
			else if(Object_IsFloat(top))
			{
				double f = Object_GetFloat(top);
				top = Object_FromFloat(-f, heap, error);
			}
			else
			{
				Error_Report(error, ErrorType_RUNTIME, "Negation operand on a non-numeric object");
				return 0;
			}

			if(top == NULL)
				return 0;

			return Runtime_Push(runtime, error, top);
		}

		case OPCODE_NOT:
		{
			ASSERT(opc == 0);

			Object *top;
			if(!Runtime_Pop(runtime, error, &top, 1))
				return 0;

			if(!Object_IsBool(top))
			{
				Error_Report(error, ErrorType_RUNTIME, "NOT operand isn't a boolean");
				return 0;
			}

			_Bool v = Object_GetBool(top);

			Object *negated = Object_FromBool(!v, heap, error);
			if(negated == NULL)
				return 0;

			return Runtime_Push(runtime, error, negated);
		}

		case OPCODE_NLB:
		{
			ASSERT(opc == 0);

			Object *top;
			if(!Runtime_Pop(runtime, error, &top, 1))
				return 0;

			Object *nullable = Object_NewNullable(top, heap, error);
			if(nullable == NULL)
				return 0;

			return Runtime_Push(runtime, error, nullable);
		}

		case OPCODE_STP:
		{
			ASSERT(opc == 0);
			Object *objs[2];
			if(!Runtime_Pop(runtime, error, objs, 2))
				return 0;
			Object *res = Object_NewSum(objs[1], objs[0], heap, error);
			if(res == NULL)
				return 0;
			return Runtime_Push(runtime, error, res);
		}

		case OPCODE_ADD:
		case OPCODE_SUB:
		case OPCODE_MUL:
		case OPCODE_DIV:
		{
			ASSERT(opc == 0);
			Object *objs[2];
			if(!Runtime_Pop(runtime, error, objs, 2))
				return 0;
			Object *res = do_math_op(objs[1], objs[0], opcode, heap, error);
			if(res == NULL)
				return 0;
			return Runtime_Push(runtime, error, res);
		}

		case OPCODE_MOD:
		{
			ASSERT(opc == 0);
			Object *objs[2];
			if(!Runtime_Pop(runtime, error, objs, 2))
				return 0;
			if (!Object_IsInt(objs[0]) || !Object_IsInt(objs[1])) {
				Error_Report(error, ErrorType_RUNTIME, "Arithmetic operation on a non-numeric object");
				return 0;
			}

			long long int x, y, z;
			y = Object_GetInt(objs[0]);
			x = Object_GetInt(objs[1]);
			z = x % y;

			Object *res = Object_FromInt(z, heap, error);
			if(res == NULL)
				return 0;

			return Runtime_Push(runtime, error, res);
		}

		case OPCODE_EQL:
		case OPCODE_NQL:
		{
			ASSERT(opc == 0);
			Object *objs[2];
			if(!Runtime_Pop(runtime, error, objs, 2))
				return 0;

			_Bool rawres = Object_Compare(objs[1], objs[0], error);
			if(error->occurred == 1)
				return 0;

			if(opcode == OPCODE_NQL)
				rawres = !rawres;

			Object *res = Object_FromBool(rawres, heap, error);
			if(res == NULL)
				return 0;

			return Runtime_Push(runtime, error, res);
		}

		case OPCODE_LSS:
		case OPCODE_GRT:
		case OPCODE_LEQ:
		case OPCODE_GEQ:
		{
			ASSERT(opc == 0);

			Object *objs[2];
			if(!Runtime_Pop(runtime, error, objs, 2))
				return 0;

			Object *res = do_relational_op(objs[1], objs[0], opcode, heap, error);
			if(res == NULL)
				return 0;

			return Runtime_Push(runtime, error, res);
		}

		case OPCODE_ASS:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_STRING);
			const char *name = ops[0].as_string;

			Object *value = Runtime_Top(runtime, 0);
			if(value == NULL) {
				Error_Report(error, ErrorType_INTERNAL, "Frame has not enough values on the stack");
				return 0;
			}
			return Runtime_SetVariable(runtime, error, name, value);
		}

		case OPCODE_POP:
		{
			ASSERT(opc == 1);
			return Runtime_Pop(runtime, error, NULL, ops[0].as_int);
		}

		case OPCODE_CHECKTYPE:
		{
			ASSERT(opc == 2);
			ASSERT(ops[0].type == OPTP_INT);
			ASSERT(ops[1].type == OPTP_STRING);
			
			const char *arg_name; 
			int arg_index;
			
			arg_index = ops[0].as_int;
			arg_name  = ops[1].as_string;
			ASSERT(arg_name != NULL);

			Object *typ = Runtime_Top(runtime, 0);
			Object *arg = Runtime_Top(runtime, -1);
			if(typ == NULL || arg == NULL)
			{
				Error_Report(error, ErrorType_INTERNAL, "Frame doesn't own enough objects to execute CHECKTYPE");
				return 0;
			}

			// Pop type
			if(!Runtime_Pop(runtime, error, NULL, 1))
				return 0;

			if (!Object_IsTypeOf(typ, arg, heap, error)) {
				char provided[512];
				char allowed[512];
				FILE *provided_fp = fmemopen(provided, sizeof(provided), "wb");
				FILE  *allowed_fp = fmemopen(allowed, sizeof(allowed), "wb");
				// TODO: Check for errors from [fmemopen]
				Object_Print(typ, allowed_fp);
				Object_Print(arg, provided_fp);
				fclose(allowed_fp);
				fclose(provided_fp);
				Error_Report(error, ErrorType_RUNTIME, "Argument %d \"%s\" has an unallowed type. Was expected something with type %s but was provided %s", 
							 arg_index+1, arg_name, allowed, provided);
				return 0;
			}
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

			Object *callable;
			if (!Runtime_Pop(runtime, error, &callable, 1)) {
				Error_Report(error, ErrorType_INTERNAL, "Frame doesn't own enough objects to execute call");
				return 0;
			}

			Object *argv[32];

			int max_argc = sizeof(argv) / sizeof(argv[0]);
			if(argc > max_argc) {
				Error_Report(error, ErrorType_INTERNAL, "Static buffer only allows function calls with up to %d arguments", max_argc);
				return 0;
			}
			if (!Runtime_Pop(runtime, error, argv, argc))
				return 0;

			Object *rets[8];
			int num_rets = Object_Call(callable, argv, argc, rets, heap, error);
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
		case OPCODE_SELECT2:
		{
			ASSERT(opc == 0);

			int to_be_popped = (opcode == OPCODE_SELECT) ? 2 : 1;

			Object *col = Runtime_Top(runtime, -1);
			Object *key = Runtime_Top(runtime, 0);
			if (col == NULL || key == NULL) {
				const char *name = "SELECT";
				if (opcode == OPCODE_SELECT2)
					name = "SELECT2";
				Error_Report(error, ErrorType_INTERNAL, "Frame has not enough values on the stack to run %s instruction", name);
				return 0;
			}
			if(!Runtime_Pop(runtime, error, NULL, to_be_popped))
				return 0;

			Error dummy;
			Error_Init(&dummy); // We want to catch the error reported by this Object_Select.

			Object *val = Object_Select(col, key, heap, &dummy);

			if(val == NULL) {
				Error_Free(&dummy);

				val = Object_NewNone(heap, error);
				if(val == NULL)
					return 0;
			}

			return Runtime_Push(runtime, error, val);
		}

		case OPCODE_INSERT:
		{
			ASSERT(opc == 0);

			Object *col = Runtime_Top(runtime, -2);
			Object *key = Runtime_Top(runtime, -1);
			Object *val = Runtime_Top(runtime,  0);
			if (col == NULL || key == NULL || val == NULL) {
				Error_Report(error, ErrorType_INTERNAL, "Frame has not enough values on the stack to run INSERT instruction");
				return 0;
			}
			if(!Runtime_Pop(runtime, error, NULL, 2))
				return 0;

			return Object_Insert(col, key, val, heap, error);
		}
		
		case OPCODE_INSERT2:
		{
			ASSERT(opc == 0);

			Object *val = Runtime_Top(runtime, -2);
			Object *col = Runtime_Top(runtime, -1);
			Object *key = Runtime_Top(runtime,  0);
			if (val == NULL || col == NULL || key == NULL) {
				Error_Report(error, ErrorType_INTERNAL, "Frame has not enough values on the stack to run INSERT2 instruction");
				return 0;
			}
			if(!Runtime_Pop(runtime, error, NULL, 2))
				return 0;

			return Object_Insert(col, key, val, heap, error);
		}

		case OPCODE_PUSHINT:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_INT);

			Object *obj = Object_FromInt(ops[0].as_int, heap, error);
			if(obj == NULL)
				return 0;

			return Runtime_Push(runtime, error, obj);
		}
		
		case OPCODE_PUSHFLT:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_FLOAT);

			Object *obj = Object_FromFloat(ops[0].as_float, heap, error);
			if(obj == NULL)
				return 0;

			return Runtime_Push(runtime, error, obj);
		}

		case OPCODE_PUSHSTR:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_STRING);

			Object *obj = Object_FromString(ops[0].as_string, -1, heap, error);
			if(obj == NULL)
				return 0;

			return Runtime_Push(runtime, error, obj);
		}

		case OPCODE_PUSHVAR:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_STRING);

			Object *value;
			if (!Runtime_GetVariable(runtime, error, ops[0].as_string, &value))
				return 0;

			if (value == NULL) {
				Error_Report(error, ErrorType_RUNTIME, "Reference to undefined variable \"%s\"", ops[0].as_string);
				return 0;
			}

			return Runtime_Push(runtime, error, value);
		}

		case OPCODE_PUSHNNE:
		{
			ASSERT(opc == 0);
			Object *obj = Object_NewNone(heap, error);
			if(obj == NULL)
				return 0;
			return Runtime_Push(runtime, error, obj);
		}

		case OPCODE_PUSHTRU:
		{
			ASSERT(opc == 0);
			Object *obj = Object_FromBool(1, heap, error);
			if(obj == NULL)
				return 0;
			return Runtime_Push(runtime, error, obj);
		}

		case OPCODE_PUSHFLS:
		{
			ASSERT(opc == 0);
			Object *obj = Object_FromBool(0, heap, error);
			if(obj == NULL) 
				return 0;
			return Runtime_Push(runtime, error, obj);
		}

		case OPCODE_PUSHFUN:
		{
			ASSERT(opc == 3);
			ASSERT(ops[0].type == OPTP_IDX);
			ASSERT(ops[1].type == OPTP_INT);
			ASSERT(ops[2].type == OPTP_STRING);

			Object *locals  = Runtime_GetLocals(runtime);
			Object *old_closure = Runtime_GetClosure(runtime);
			Object *new_closure = Object_NewClosure(old_closure, locals, heap, error); // Should old_closure and locals be in the reverse order?
			if(new_closure == NULL)
				return 0;

			Object *func = Object_FromNojaFunction(runtime, ops[2].as_string, exe, ops[0].as_int, ops[1].as_int, new_closure, heap, error);
			if(func == NULL)
				return 0;

			return Runtime_Push(runtime, error, func);
		}

		case OPCODE_PUSHLST:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_INT);

			Object *obj = Object_NewList(ops[0].as_int, heap, error);
			if(obj == NULL)
				return 0;

			return Runtime_Push(runtime, error, obj);
		}

		case OPCODE_PUSHMAP:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_INT);

			Object *obj = Object_NewMap(ops[0].as_int, heap, error);
			if(obj == NULL)
				return 0;

			return Runtime_Push(runtime, error, obj);
		}

		case OPCODE_PUSHNNETYP:
		{
			ASSERT(opc == 0);

			Object *obj = (Object*) Object_GetNoneType();
			ASSERT(obj != NULL);

			return Runtime_Push(runtime, error, obj);
		}

		case OPCODE_PUSHTYP:
		{
			ASSERT(opc == 0);

			Object *top = Runtime_Top(runtime, 0);
			if (top == NULL) {
				Error_Report(error, ErrorType_INTERNAL, "Frame has not enough values on the stack to run PUSHTYP instruction");
				return 0;
			}

			Object *typ = (Object*) Object_GetType(top);
			ASSERT(typ != NULL);
			
			return Runtime_Push(runtime, error, typ);
		}

		case OPCODE_EXIT:
		{
			ASSERT(opc == 0);
			Object *vars = Runtime_GetLocals(runtime);
			ASSERT(vars != NULL);
			Runtime_Push(runtime, error, vars);
			return 0;
		}

		case OPCODE_RETURN:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_INT);
			int retc = ops[0].as_int;
			UNUSED(retc);
			ASSERT(retc >= 0);
			ASSERT(retc <= MAX_RETS);
			ASSERT((size_t) retc == Runtime_GetFrameStackUsage(runtime));
			return 0;
		}

		case OPCODE_JUMP:
		ASSERT(opc == 1);
		ASSERT(ops[0].type == OPTP_IDX);
		Runtime_SetInstructionIndex(runtime, ops[0].as_int);
		return 1;

		case OPCODE_JUMPIFANDPOP:
		case OPCODE_JUMPIFNOTANDPOP:
		{
			ASSERT(opc == 1);
			ASSERT(ops[0].type == OPTP_IDX);				
			long long int target = ops[0].as_int;

			Object *top;
			if(!Runtime_Pop(runtime, error, &top, 1))
				return 0;			

			if(!Object_IsBool(top)) {
				Error_Report(error, ErrorType_RUNTIME, "Not a boolean");
				return 0;
			}

			if(( Object_GetBool(top) && opcode == OPCODE_JUMPIFANDPOP) 
			|| (!Object_GetBool(top) && opcode == OPCODE_JUMPIFNOTANDPOP))
				Runtime_SetInstructionIndex(runtime, target);
			return 1;
		}

		default:
		UNREACHABLE;
		return 0;
	}

	return 1;
}

static bool runInstructionsUntilSomethingHappens(Runtime *runtime, Error *error)
{
	Heap *heap = Runtime_GetHeap(runtime);
	RuntimeCallback callback = Runtime_GetCallback(runtime);

	if(Runtime_WasInterrupted(runtime) || (callback.func != NULL && !callback.func(runtime, callback.data)))
		Error_Report(error, ErrorType_RUNTIME, "Forced abortion");
	else
		while(runInstruction(runtime, error))
		{
			if(Runtime_WasInterrupted(runtime) || (callback.func != NULL && !callback.func(runtime, callback.data)))
			{
				Error_Report(error, ErrorType_RUNTIME, "Forced abortion");
				break;
			}

			if(Heap_GetUsagePercentage(heap) > 100)
				if(!Runtime_CollectGarbage(runtime, error))
					break;
		}

	// If an error occurred, we want to return NULL.
	return !error->occurred;
}

static int runExecutableAtIndex(Runtime *runtime, Error *error,
		    				    Executable *exe, int index,
		    				    Object *closure,
		    				    Object *rets[static MAX_RETS],
						        Object *argv[], int argc)
{
	if (!Runtime_PushFrame(runtime, error, closure, exe, index))
    	return -1;

    for (int i = 0; i < argc; i++)
    	if (!Runtime_Push(runtime, error, argv[i]))
    		return -1;

    if (!runInstructionsUntilSomethingHappens(runtime, error))
    	return -1;

    // Get return values
    int retc = 0;
    while (retc < MAX_RETS && (rets[retc] = Runtime_Top(runtime, -retc)))
    	retc++;

    if (!Runtime_PopFrame(runtime))
		return -1;

	return retc;
}

int runSource(Runtime *runtime, Source *source, Object *rets[static MAX_RETS], Error *error)
{
	int error_offset;
	Executable *exe = compile(source, error, &error_offset);
    if(exe == NULL) {
    	Error suberror;
    	Error_Init(&suberror);
    	Runtime_PushFailedFrame(runtime, &suberror, source, error_offset); // If this fails, there's nothing we can do
        Error_Free(&suberror);
        return -1;
    }

    int retc = runExecutableAtIndex(runtime, error, exe, 0, NULL, rets, NULL, 0);

    Executable_Free(exe);
    return retc;
}

int runBytecodeSource(Runtime *runtime, Source *source, Object *rets[static MAX_RETS], Error *error)
{
	int error_offset;
	Executable *exe = assemble(source, error, &error_offset);
    if(exe == NULL) {
    	Error suberror;
    	Error_Init(&suberror);
    	Runtime_PushFailedFrame(runtime, &suberror, source, error_offset); // If this fails, there's nothing we can do
    	Error_Free(&suberror);
        return -1;
    }

    int retc = runExecutableAtIndex(runtime, error, exe, 0, NULL, rets, NULL, 0);
    
    Executable_Free(exe);
    return retc;
}

int runFileEx(Runtime *runtime, const char *file, Object *rets[static MAX_RETS], Error *error)
{
	Source *source = Source_FromFile(file, error);
	if (source == NULL)
		return -1;

	int retc = runSource(runtime, source, rets, error);

	Source_Free(source);
	return retc;
}

int runStringEx(Runtime *runtime, const char *name, const char *string, Object *rets[static MAX_RETS], Error *error)
{
	Source *source = Source_FromString(name, string, -1, error);
	if (source == NULL)
		return -1;
	
	int retc = runSource(runtime, source, rets, error);
	
	Source_Free(source);
	return retc;
}

int runBytecodeFileEx(Runtime *runtime, const char *file, Object *rets[static MAX_RETS], Error *error)
{
	Source *source = Source_FromFile(file, error);
	if (source == NULL)
		return -1;

	int retc = runBytecodeSource(runtime, source, rets, error);

	Source_Free(source);
	return retc;
}

int runBytecodeStringEx(Runtime *runtime, const char *name, const char *string, Object *rets[static MAX_RETS], Error *error)
{
	Source *source = Source_FromString(name, string, -1, error);
	if (source == NULL)
		return -1;
	
	int retc = runBytecodeSource(runtime, source, rets, error);
	
	Source_Free(source);
	return retc;
}

bool runFile(Runtime *runtime, const char *file, Error *error)
{
	Object *rets[MAX_RETS];
	return runFileEx(runtime, file, rets, error) >= 0;
}

bool runString(Runtime *runtime, const char *string, Error *error)
{
	Object *rets[MAX_RETS];
	return runStringEx(runtime, "(unnamed)", string, rets, error) >= 0;
}

bool runBytecodeFile(Runtime *runtime, const char *file, Error *error)
{
	Object *rets[MAX_RETS];
	return runBytecodeFileEx(runtime, file, rets, error) >= 0;
}

bool runBytecodeString(Runtime *runtime, const char *string, Error *error)
{
	Object *rets[MAX_RETS];
	return runBytecodeStringEx(runtime, "(unnamed)", string, rets, error) >= 0;
}

static bool makePathRelativeToScript(Runtime *runtime, const char *src_path, char *dst_path, size_t dst_size)
{
	size_t src_size = strlen(src_path);

	if(Path_IsAbsolute(src_path)) {
		
		if(src_size >= dst_size)
			return false;
		
		strcpy(dst_path, src_path);
	
	} else {

		size_t written = Runtime_GetCurrentScriptFolder(runtime, dst_path, dst_size);
		if(written == 0)
			return false;

		if(written + src_size >= dst_size)
			return false;

		memcpy(dst_path + written, src_path, src_size);
		dst_path[written + src_size] = '\0';
	}

	return true;
}

int runFileRelativeToScript(Runtime *runtime, const char *file, Object *rets[static MAX_RETS], Error *error)
{
	char full[1024];
	if (!makePathRelativeToScript(runtime, file, full, sizeof(full))) {
		Error_Report(error, ErrorType_INTERNAL, "Internal buffer is too small");
		return -1;
	}

	Source *source = Source_FromFile(full, error);
	if (source == NULL)
		return -1;

	int retc = runSource(runtime, source, rets, error);

	Source_Free(source);
	return retc;
}
