
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

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef NULL
#define NULL ((void*) 0)
#endif

#define UNUSED(x) ((void) (x))

#ifdef DEBUG
#define ASSERT(X)       \
    do {                \
        if(!(X)) {      \
            fprintf(stderr, "Assertion failure %s:%d (in %s): [" #X "] is false\n", __FILE__, __LINE__, __func__); \
            abort();    \
        }               \
    } while(0);
#define UNREACHABLE     \
    do {                \
        if(!(x)) {      \
            fprintf(stderr, "ABORT at %s:%d (in %s): Reached code assumed to be unreachable\n", __FILE__, __LINE__, __func__); \
            abort();    \
        }               \
    } while(0);
#else
#define ASSERT(x)
#define UNREACHABLE
#endif

#define membersizeof(type, member) (sizeof(((type*) 0)->member))