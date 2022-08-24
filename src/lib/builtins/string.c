#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "string.h"
#include "../utils/defs.h"
#include "../utils/utf8.h"
#include "../runtime/runtime.h"

static int bin_ord(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    UNUSED(runtime);
    UNUSED(error);
    UNUSED(argc);
    ASSERT(argc == 1);

    uint32_t ret = 0;

    if(!Object_IsString(argv[0]))
    {
        Error_Report(error, 0, "Argument #%d is not a string", 1);
        return -1;
    }

    const char *string;
    size_t      length;
    string = Object_GetString(argv[0], &length);
    ASSERT(string != NULL);

    if(length == 0)
    {
        Error_Report(error, 0, "Argument #%d is an empty string", 1);
        return -1;
    }


    int k = utf8_sequence_to_utf32_codepoint(string, length, &ret);
    UNUSED(k);
    ASSERT(k >= 0);

    Object *temp = Object_FromInt(ret, Runtime_GetHeap(runtime), error);

    if(temp == NULL)
        return -1;

    rets[0] = temp;
    return 1;
}

static int bin_chr(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    UNUSED(argc);
    ASSERT(argc == 1);

    if(!Object_IsInt(argv[0]))
    {
        Error_Report(error, 0, "Argument #%d is not an integer", 1);
        return -1;
    }

    char buff[32];
    
    int value = Object_GetInt(argv[0]);
    
    int k = utf8_sequence_from_utf32_codepoint(buff,sizeof(buff),value);

    if(k<0)
    {
        Error_Report(error, 0, "Argument #%d is not valid utf-32", 1);
        return -1;
    }
    
    Object *temp = Object_FromString(buff,k,Runtime_GetHeap(runtime),error);

    if(temp == NULL)
        return -1;

    rets[0] = temp;
    return 1;
}

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
        size_t length;
        const char *s = Object_GetString(argv[i], &length);

        memcpy(buffer + written, s, length);
        written += length;
    }

    buffer[total_count] = '\0';

    result = Object_FromString(buffer, total_count, Runtime_GetHeap(runtime), error);
        
    if(starting != buffer)
        free(buffer);

    if(result == NULL)
        return -1;

    rets[0] = result;
    return 1;
}

StaticMapSlot bins_string[] = {
    { "ord", SM_FUNCT, .as_funct = bin_ord, .argc = 1 },
    { "chr", SM_FUNCT, .as_funct = bin_chr, .argc = 1 },
    { "cat", SM_FUNCT, .as_funct = bin_cat, .argc = -1 },
};
