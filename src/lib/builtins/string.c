#include <string.h>
#include <stdlib.h>
#include "string.h"
#include "../utils/defs.h"
#include "../runtime/runtime.h"

static int bin_cat(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    unsigned int total_count = 0;

    for(unsigned int i = 0; i < argc; i += 1)
    {
        if(!Object_IsString(argv[i]))
        {
            Error_Report(error, 0, "Argument #%d is not a string", i+1);
            return -1;
        }

        total_count += Object_Count(argv[i], error);

        if(error->occurred)
            return -1;
    }

    char starting[128];
    char *buffer = starting;

    if(total_count > sizeof(starting)-1)
    {
        buffer = malloc(total_count+1);

        if(buffer == NULL)
        {
            Error_Report(error, 1, "No memory");
            return -1;
        }
    }

    Object *result = NULL;

    for(unsigned int i = 0, written = 0; i < argc; i += 1)
    {
        int n;
        const char *s = Object_ToString(argv[i], &n, 
            Runtime_GetHeap(runtime), error);

        if(error->occurred)
            goto done;

        memcpy(buffer + written, s, n);
        written += n;
    }

    buffer[total_count] = '\0';

    result = Object_FromString(buffer, total_count, Runtime_GetHeap(runtime), error);

done:
    if(starting != buffer)
        free(buffer);

    if(result == NULL)
        return -1;

    rets[0] = result;
    return 1;
}

StaticMapSlot bins_string[] = {
    { "cat", SM_FUNCT, .as_funct = bin_cat, .argc = -1 },
};
