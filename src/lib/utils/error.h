
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

#ifndef ERROR_H
#define ERROR_H
#include <stdarg.h>

typedef enum {
	ErrorType_SYNTAX,
	ErrorType_SEMANTIC,
	ErrorType_RUNTIME,
	ErrorType_INTERNAL,
	ErrorType_UNSPECIFIED = 0,
} ErrorType;

typedef struct Error Error;
struct Error {
	ErrorType type;
	void 		(*on_report)(Error *err);
	_Bool  		occurred, 
				internal, 
				truncated;
	int    		length;
	char*		message;
	char   		message2[512];
	const char *file,
			   *func;
	int 		line;
};

void 	Error_Init(Error *err);
void 	Error_Init2(Error *err, void (*on_report)(Error *err));
void 	Error_Free(Error *err);
#define Error_Report(err, typ, fmt, ...) _Error_Report(err, typ, __FILE__, __func__, __LINE__, fmt, ## __VA_ARGS__)
void   _Error_Report (Error *err, ErrorType typ, const char *file, const char *func, int line, const char *fmt, ...);
void   _Error_Report2(Error *err, ErrorType typ, const char *file, const char *func, int line, const char *fmt, va_list va);
void    Error_Panic_(const char *file, int line, const char *fmt, ...);
#define Error_Panic(fmt, ...) Error_Panic_(__FILE__, __LINE__, fmt, ## __VA_ARGS__)
void    Error_Print(Error *error, ErrorType type_if_unspecified);
#endif