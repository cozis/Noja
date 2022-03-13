
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

#ifndef RUNTIME_ERROR_H
#define RUNTIME_ERROR_H
#include "../utils/error.h"
#include "runtime.h"

typedef struct {
	Error base;
	Runtime *runtime;
	Snapshot *snapshot;
} RuntimeError;

void RuntimeError_Init(RuntimeError *error, Runtime *runtime);
void RuntimeError_Free(RuntimeError *error);
#endif