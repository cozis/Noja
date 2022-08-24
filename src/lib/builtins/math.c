
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

#include <math.h>
#include "math.h"
#include "../utils/defs.h"

#define WRAP_FUNC(name) \
	static int bin_ ## name(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error) \
	{											\
		UNUSED(argc);							\
		ASSERT(argc == 1);						\
												\
		if(Object_IsFloat(argv[0]))				\
		{										\
			double v = Object_GetFloat(argv[0]); \
												\
			rets[0] = Object_FromFloat(name(v), Runtime_GetHeap(runtime), error); \
			if(rets[0] == NULL)					\
				return -1;						\
			return 1;							\
		}										\
		else 									\
		{										\
			Error_Report(error, 0, "Expected first argument to be a float, but it's a %s", Object_GetName(argv[0])); \
			return -1;							\
		}										\
	}

#define WRAP_FUNC_2(name) \
	static int bin_ ## name(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error) \
	{											\
		UNUSED(argc);							\
		ASSERT(argc == 2);						\
												\
		if(!Object_IsFloat(argv[0]))			\
		{										\
			Error_Report(error, 0, "Expected first argument to be a float, but it's a %s", Object_GetName(argv[0])); \
			return -1;							\
		}										\
												\
		if(!Object_IsFloat(argv[1]))			\
		{										\
			Error_Report(error, 0, "Expected second argument to be a float, but it's a %s", Object_GetName(argv[1])); \
			return -1;							\
		}										\
												\
		double v1 = Object_GetFloat(argv[0]); \
		double v2 = Object_GetFloat(argv[1]); \
												\
		Object *res = Object_FromFloat(name(v1, v2), Runtime_GetHeap(runtime), error); \
		if(res == NULL) return -1;				\
		rets[0] = res;							\
		return 1;								\
	}

WRAP_FUNC(ceil)
WRAP_FUNC(floor)
WRAP_FUNC(sin)
WRAP_FUNC(cos)
WRAP_FUNC(tan)
WRAP_FUNC(asin)
WRAP_FUNC(acos)
WRAP_FUNC(atan)
WRAP_FUNC(exp)
WRAP_FUNC(log)
WRAP_FUNC(log10)
WRAP_FUNC(sqrt)
WRAP_FUNC_2(atan2)
WRAP_FUNC_2(pow)

StaticMapSlot bins_math[] = {
	{ "PI", SM_FLOAT, .as_float = M_PI },
	{ "E",  SM_FLOAT, .as_float = M_E },

	{ "floor", SM_FUNCT, .as_funct = bin_floor, .argc = 1, },
	{ "ceil",  SM_FUNCT, .as_funct = bin_ceil,  .argc = 1, },

	{ "cos", SM_FUNCT, .as_funct = bin_cos, .argc = 1, },
	{ "sin", SM_FUNCT, .as_funct = bin_sin, .argc = 1, },
	{ "tan", SM_FUNCT, .as_funct = bin_tan, .argc = 1, },

	{ "acos",  SM_FUNCT, .as_funct = bin_acos, .argc = 1, },
	{ "asin",  SM_FUNCT, .as_funct = bin_asin, .argc = 1, },
	{ "atan",  SM_FUNCT, .as_funct = bin_atan, .argc = 1, },
	{ "atan2", SM_FUNCT, .as_funct = bin_atan2, .argc = 2, },

	{ "exp",   SM_FUNCT, .as_funct = bin_exp, .argc = 1, },
	{ "log",   SM_FUNCT, .as_funct = bin_log, .argc = 1, },
	{ "log10", SM_FUNCT, .as_funct = bin_log10, .argc = 1, },

	{ "pow",  SM_FUNCT, .as_funct = bin_pow,  .argc = 1, },
	{ "sqrt", SM_FUNCT, .as_funct = bin_sqrt, .argc = 1, },
	{ NULL, SM_END, {}, {} },
};
