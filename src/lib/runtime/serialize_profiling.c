#include "serialize_profiling.h"

void Runtime_SerializeProfilingResultsToStream(Runtime *runtime, FILE *stream)
{
    TimingTable *table = Runtime_GetTimingTable(runtime);
    if (table == NULL)
        return; // Runtime wasn't profiling

    size_t count;
    const FunctionExecutionSummary *summary = TimingTable_getSummary(table, &count);

    for (size_t i = 0; i < count; i++) {
        if (summary[i].calls > 0) {
            fprintf(stream, "%20s - %s - %ld calls - %.2lfus\n",
                   summary[i].name,
                   Source_GetName(summary[i].src),
                   summary[i].calls,
                   summary[i].time * 1000000);
        }
    }
}

bool Runtime_SerializeProfilingResultsToFile(Runtime *runtime, const char *file)
{
    FILE *stream = fopen(file, "wb");
    if (stream == NULL)
        return false;
    Runtime_SerializeProfilingResultsToStream(runtime, stream);
    fclose(stream);
    return true;
}