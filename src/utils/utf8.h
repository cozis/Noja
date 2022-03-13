
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

#ifndef UTF8_H
#define UTF8_H
#include <stdint.h> // uint32_t
int utf8_sequence_from_utf32_codepoint(char *utf8_data, int nbytes, uint32_t utf32_code);
int utf8_sequence_to_utf32_codepoint(const char *utf8_data, int nbytes, uint32_t *utf32_code);
int utf8_strlen(const char *utf8_data, int nbytes);
int utf8_prev(const char *utf8_data, int nbytes, int idx, uint32_t *utf32_code);
int utf8_next(const char *utf8_data, int nbytes, int idx, uint32_t *utf32_code);
int utf8_curr(const char *utf8_data, int nbytes, int idx, uint32_t *utf32_code);
#endif // #ifndef UTF8_H