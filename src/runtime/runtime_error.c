
/* Copyright (c) Francesco Cozzuto <francesco.cozzuto@gmail.com>
**
** This file is part of The Noja Interpreter.
**
** The Noja Interpreter is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at 
** your option) any later version.
**
** The Noja Interpreter is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of 
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General 
** Public License for more details.
**
** You should have received a copy of the GNU General Public License along 
** with The Noja Interpreter. If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include "runtime_error.h"

static void on_report(Error *error)
{
	assert(error != NULL);

	RuntimeError *error2 = (RuntimeError*) error;

	if(error2->runtime != NULL)
		error2->snapshot = Snapshot_New(error2->runtime);
}

void RuntimeError_Init(RuntimeError *error, Runtime *runtime)
{
	assert(error != NULL);

	Error_Init2(&error->base, on_report);

	error->runtime = runtime;
	error->snapshot = NULL;
}

void RuntimeError_Free(RuntimeError *error)
{
	if(error->snapshot)
		Snapshot_Free(error->snapshot);
	Error_Free(&error->base);
}