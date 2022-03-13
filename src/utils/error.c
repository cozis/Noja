
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "error.h"

void Error_Init(Error *err)
{
	memset(err, 0, sizeof (Error));
}

void Error_Init2(Error *err, void (*on_report)(Error *err))
{
	memset(err, 0, sizeof (Error));
	err->on_report = on_report;
}

void Error_Free(Error *err)
{
	if(err->message2 != err->message)
		free(err->message);
	memset(err, 0, sizeof (Error));
}

void _Error_Report(Error *err, _Bool internal, 
	const char *file, const char *func, int line, 
	const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	_Error_Report2(err, internal, file, func, line, fmt, va);
	va_end(va);
}

void _Error_Report2(Error *err, _Bool internal, 
	const char *file, const char *func, int line, 
	const char *fmt, va_list va)
{
	assert(err);
	assert(file);
	assert(func);
	assert(line > 0);
	assert(fmt);
	assert(err->occurred == 0);

	err->occurred = 1;
	err->internal = internal;
	err->file = file;
	err->func = func;
	err->line = line;

	va_list va2;
	va_copy(va2, va);

	int p = vsnprintf(err->message2, sizeof(err->message2), fmt, va);

	assert(p > -1);

	if((unsigned int) p > sizeof(err->message2)-1)
		{
			char *temp = malloc(p+1);

			if(temp == NULL)
				{
					err->truncated = 1;
					err->message   = err->message2;
					err->length    = sizeof(err->message2)-1;
				}
			else
				{
					snprintf(temp, p+1, fmt, va2);
					err->truncated = 0;
					err->message   = temp;
					err->length    = p;
				}
		}
	else
		{
			err->truncated = 0;
			err->message   = err->message2;
			err->length    = p;
		}

	va_end(va2);

	if(err->on_report)
		err->on_report(err);
}