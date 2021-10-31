#include <assert.h>
#include "runtime_error.h"

static void on_report(Error *error)
{
	assert(error != NULL);

	RuntimeError *error2 = (RuntimeError*) error;

	if(error2->runtime != NULL)
		error2->snapshot = Snapshot_New(error2->runtime);
}

void RuntimeError_Init(RuntimeError *error, Runtime *runtime)
{
	assert(error != NULL);

	Error_Init2(&error->base, on_report);

	error->runtime = runtime;
	error->snapshot = NULL;
}

void RuntimeError_Free(RuntimeError *error)
{
	if(error->snapshot)
		Snapshot_Free(error->snapshot);
	Error_Free(&error->base);
}