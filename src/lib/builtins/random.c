#include <stdlib.h>
#include "utils.h"
#include "random.h"
#include "../utils/defs.h"

static int bin_seed(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	UNUSED(argv);
	ASSERT(argc == 1);

	ParsedArgument pargs[1];
	if (!parseArgs(error, argv, argc, pargs, "i"))
		return -1;
	
	int64_t seed = pargs[0].as_int;
	srand(seed);
	return returnValues2(error, runtime, rets, "n");
}

static int bin_generate(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	UNUSED(argc);
	UNUSED(argv);
	ASSERT(argc == 2);

	ParsedArgument pargs[2];
	if (!parseArgs(error, argv, argc, pargs, "?i?i"))
		return -1;
	
	int64_t min, max;
	if (pargs[0].defined) min = pargs[0].as_int; else min = 0;
	if (pargs[1].defined) max = pargs[1].as_int; else max = INT64_MAX;
	int64_t value = min + (max - min) * (double) rand() / RAND_MAX;
	return returnValues2(error, runtime, rets, "i", value);
}

StaticMapSlot bins_random[] = {
	{"seed",     SM_FUNCT, .as_funct = bin_seed,     .argc = 1},
	{"generate", SM_FUNCT, .as_funct = bin_generate, .argc = 2},
	{ NULL, SM_END, {}, {} },
};