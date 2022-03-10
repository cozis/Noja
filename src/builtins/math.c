#include <assert.h>
#include <math.h>
#include "math.h"

#define WRAP_FUNC(name) \
	static Object *bin_ ## name(Runtime *runtime, Object **argv, unsigned int argc, Error *error) 	\
	{																								\
		assert(argc == 1);																			\
																									\
		if(Object_IsFloat(argv[0]))																	\
			{																						\
				double v = Object_ToFloat(argv[0], error);											\
																									\
				if(error->occurred) 																\
					return NULL;																	\
																									\
				return Object_FromFloat(name(v), Runtime_GetHeap(runtime), error);					\
			}																						\
		else 																						\
			{																						\
				Error_Report(error, 0, "Expected first argument to be a float, but it's a %s", Object_GetName(argv[0]));\
				return NULL;																		\
			}																						\
	}

#define WRAP_FUNC_2(name) \
	static Object *bin_ ## name(Runtime *runtime, Object **argv, unsigned int argc, Error *error) 	\
	{																								\
		assert(argc == 2);																			\
																									\
		if(!Object_IsFloat(argv[0]))																\
			{																						\
				Error_Report(error, 0, "Expected first argument to be a float, but it's a %s", Object_GetName(argv[0]));\
				return NULL;																		\
			}																						\
																									\
		if(!Object_IsFloat(argv[1]))																\
			{																						\
				Error_Report(error, 0, "Expected second argument to be a float, but it's a %s", Object_GetName(argv[1]));\
				return NULL;																		\
			}																						\
																									\
		double v1 = Object_ToFloat(argv[0], error);													\
																									\
		if(error->occurred) 																		\
			return NULL;																			\
																									\
		double v2 = Object_ToFloat(argv[1], error);													\
																									\
		if(error->occurred) 																		\
			return NULL;																			\
																									\
		return Object_FromFloat(name(v1, v2), Runtime_GetHeap(runtime), error);						\
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

const StaticMapSlot bins_math[] = {
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