
/* Copyright (c) 2022 Francesco Cozzuto <francesco.cozzuto@gmail.com>
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

#ifndef SOURCE_H
#define SOURCE_H
#include "error.h"
typedef struct xSource Source;
Source 		*Source_Copy(Source *s);
void 		 Source_Free(Source *s);
const char  *Source_GetName(const Source *s);
const char  *Source_GetBody(const Source *s);
unsigned int Source_GetSize(const Source *s);
Source 		*Source_FromFile(const char *file, Error *error);
Source 		*Source_FromString(const char *name, const char *body, int size, Error *error);
#endif