
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

#ifndef ERROR_H
#define ERROR_H
#include <stdarg.h>

typedef struct Error Error;

struct Error {
	void 		(*on_report)(Error *err);
	_Bool  		occurred, 
				internal, 
				truncated;
	int    		length;
	char*		message;
	char   		message2[256];
	const char *file,
			   *func;
	int 		line;
};

void 	Error_Init(Error *err);
void 	Error_Init2(Error *err, void (*on_report)(Error *err));
void 	Error_Free(Error *err);
#define Error_Report(err, internal, fmt, ...) _Error_Report(err, internal, __FILE__, __func__, __LINE__, fmt, ## __VA_ARGS__)
void   _Error_Report (Error *err, _Bool internal, const char *file, const char *func, int line, const char *fmt, ...);
void   _Error_Report2(Error *err, _Bool internal, const char *file, const char *func, int line, const char *fmt, va_list va);
#endif