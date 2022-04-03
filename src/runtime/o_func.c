
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

#include <assert.h>
#include <stdlib.h>
#include "../utils/defs.h"
#include "../objects/objects.h"
#include "runtime.h"

typedef struct {
	Object base;
	Runtime *runtime;
	Executable *exe;
	int index, argc;
	Object *closure;
} FunctionObject;

static _Bool free_(Object *self, Error *error)
{
	(void) error;
	
	FunctionObject *func = (FunctionObject*) self;
	Executable_Free(func->exe);	
	return 1;
}

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp)
{
	FunctionObject *func = (FunctionObject*) self;
	callback(&func->closure, userp);
}

static Object *call(Object *self, Object **argv, unsigned int argc, Heap *heap, Error *error)
{
	assert(self != NULL && heap != NULL && error != NULL);
	
	FunctionObject *func = (FunctionObject*) self;

	assert(func->exe != NULL);
	assert(func->argc >= 0);
	assert(func->index >= 0);

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
			Error_Report(error, 1, "No memory");
			return NULL;
		}

		// Copy the provided arguments.
		for(int i = 0; i < (int) argc; i += 1)
			argv2[i] = argv[i];

		// Set the unspecified arguments to none.
		for(int i = argc; i < expected_argc; i += 1)
		{
			argv2[i] = Object_NewNone(heap, error);

			if(argv2[i] == NULL)
				return 0;
		}
	}
	else
		// The right amount of arguments was provided.
		argv2 = argv;

	Object *result = run(func->runtime, error, func->exe, func->index, func->closure, argv2, expected_argc);

	// NOTE: Every object reference is invalidated from here.

	if(argv2 != argv)
		free(argv2);

	return result;
}

static TypeObject t_func = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "function",
	.size = sizeof (FunctionObject),
	.call = call,
	.walk = walk,
	.free = free_,
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
Object *Object_FromNojaFunction(Runtime *runtime, Executable *exe, int index, int argc, Object *closure, Heap *heap, Error *error)
{
	assert(runtime != NULL);
	assert(exe != NULL);
	assert(index >= 0);
	assert(argc >= 0);
	assert(heap != NULL);
	assert(error != NULL);

	FunctionObject *func = (FunctionObject*) Heap_Malloc(heap, &t_func, error);

	if(func == NULL)
		return NULL;

	Executable *exe_copy = Executable_Copy(exe);

	if(exe_copy == NULL)
	{
		Error_Report(error, 1, "Failed to copy executable");
		return NULL;
	}

	func->runtime = runtime;
	func->exe = exe_copy;
	func->index = index;
	func->argc = argc;
	func->closure = closure;

	return (Object*) func;
}