
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "error.h"
#include "defs.h"

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

void _Error_Report(Error *err, ErrorType type, 
	const char *file, const char *func, int line, 
	const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	_Error_Report2(err, type, file, func, line, fmt, va);
	va_end(va);
}

void _Error_Report2(Error *err, ErrorType type, 
	const char *file, const char *func, int line, 
	const char *fmt, va_list va)
{
	ASSERT(err);
	ASSERT(file);
	ASSERT(func);
	ASSERT(line > 0);
	ASSERT(fmt);

#ifdef DEBUG
	if(err->occurred != 0) {
		fprintf(stderr, "Error previously reported at %s:%d (in %s) :: %s\n", err->file, err->line, err->func, err->message);
	}
#endif
	ASSERT(err->occurred == 0);

	err->occurred = 1;
	err->type = type;
	err->file = file;
	err->func = func;
	err->line = line;

	va_list va2;
	va_copy(va2, va);

	int p = vsnprintf(err->message2, sizeof(err->message2), fmt, va);

	ASSERT(p > -1);

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
			vsnprintf(temp, p+1, fmt, va2);
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

void Error_Panic_(const char *file, int line, 
	              const char *fmt, ...)
{
	FILE *fp = stderr;

	va_list args;
	va_start(args, fmt);
	fprintf(fp, "Panic: ");
	vfprintf(fp, fmt, args);
	fprintf(fp, " (reported in %s:%d)", file, line);
	va_end(args);
	abort();
}

void Error_Print(Error *error, ErrorType type_if_unspecified, FILE *stream)
{
    ErrorType type = error->type;
    if (type == ErrorType_UNSPECIFIED)
        type = type_if_unspecified;

    const char *name;
    switch (error->type) {
        case ErrorType_INTERNAL: name = "Internal Error"; break;
        case ErrorType_SYNTAX:   name =   "Syntax Error"; break;
        case ErrorType_SEMANTIC: name = "Semantic Error"; break;
        case ErrorType_RUNTIME:  name =  "Runtime Error"; break;
        default: name = "Error"; break;
    }

    fprintf(stream, "%s: %s.", name, error->message);

#ifdef DEBUG
    if(error->file != NULL)
    {
        if(error->line > 0 && error->func != NULL)
            fprintf(stream, " (Reported in %s:%d in %s)", error->file, error->line, error->func);
        else if(error->line > 0 && error->func == NULL)
            fprintf(stream, " (Reported in %s:%d)", error->file, error->line);
        else if(error->line < 1 && error->func != NULL)
            fprintf(stream, " (Reported in %s in %s)", error->file, error->func);
    }
#endif
    
    fprintf(stream, "\n");
}