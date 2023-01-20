#include "runtime.h"

void Runtime_SerializeProfilingResultsToStream(Runtime *runtime, FILE *stream);
bool Runtime_SerializeProfilingResultsToFile(Runtime *runtime, const char *file);
