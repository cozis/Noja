#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "utils.h"
#include "string.h"
#include "../utils/defs.h"
#include "../utils/utf8.h"
#include "../runtime.h"

static int bin_ord(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    UNUSED(runtime);
    UNUSED(error);
    UNUSED(argc);
    ASSERT(argc == 1);

    uint32_t ret = 0;

    if(!Object_IsString(argv[0]))
    {
        Error_Report(error, ErrorType_RUNTIME, "Argument #%d is not a string", 1);
        return -1;
    }

    const char *string;
    size_t      length;
    string = Object_GetString(argv[0], &length);
    ASSERT(string != NULL);

    if(length == 0)
    {
        Error_Report(error, ErrorType_RUNTIME, "Argument #%d is an empty string", 1);
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
        Error_Report(error, ErrorType_RUNTIME, "Argument #%d is not an integer", 1);
        return -1;
    }

    char buff[32];
    
    int value = Object_GetInt(argv[0]);
    
    int k = utf8_sequence_from_utf32_codepoint(buff,sizeof(buff),value);

    if(k<0)
    {
        Error_Report(error, ErrorType_RUNTIME, "Argument #%d is not valid utf-32", 1);
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
            Error_Report(error, ErrorType_RUNTIME, "Argument #%d is not a string", i+1);
            return -1;
        }
        
        size_t length;
        Object_GetString(argv[i], &length);
        total_count += length;

        // The Object_Count of a string doesn't match the
        // byte count, but the second argument of Object_GetString
        // does.
    }

    char starting[128];
    char *buffer = starting;

    if(total_count > sizeof(starting)-1)
    {
        buffer = malloc(total_count+1);

        if(buffer == NULL)
        {
            Error_Report(error, ErrorType_INTERNAL, "No memory");
            return -1;
        }
    }

    for(unsigned int i = 0, written = 0; i < argc; i += 1)
    {
        size_t length;
        const char *s = Object_GetString(argv[i], &length);

        memcpy(buffer + written, s, length);
        written += length;
    }

    buffer[total_count] = '\0';

    Object *result = Object_FromString(buffer, total_count, Runtime_GetHeap(runtime), error);
        
    if(starting != buffer)
        free(buffer);

    if(result == NULL)
        return -1;

    rets[0] = result;
    return 1;
}

static int bin_slice(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    // 0: string
    // 1: start offset or none
    // 2: end offset or none

    ParsedArgument pargs[3];
    if (!parseArgs(error, argv, argc, pargs, "s?i?i"))
        return -1;

    const char *srcstr = pargs[0].as_string.data;
    size_t      srclen = pargs[0].as_string.size;

    size_t offset;
    size_t length;

    if (pargs[1].defined) {
        int64_t n = pargs[1].as_int;
        if (n < 0) {
            Error_Report(error, ErrorType_RUNTIME, "starting offset of string slice must be non-negative");
            return -1;
        }
        offset = (size_t) n;
    } else
        offset = 0;

    if (pargs[2].defined) {
        int64_t n = pargs[2].as_int;
        if (n < 0) {
            Error_Report(error, ErrorType_RUNTIME, "length of string slice must be non-negative");
            return -1;
        }
        length = (size_t) n;
    } else
        length = srclen - offset;

    if (offset > srclen) {
        Error_Report(error, ErrorType_RUNTIME, "string slice offset is out of bounds");
        return -1;
    }
    if (offset + length > srclen) {
        Error_Report(error, ErrorType_RUNTIME, "string slice length is out of bounds");
        return -1;
    }

    Heap *heap = Runtime_GetHeap(runtime);
    Object *slice = Object_FromString(srcstr + offset, length, heap, error);
    if (slice == NULL)
        return -1;
    rets[0] = slice;
    return 1;
}

static int bin_trim(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    // 0: string

    ParsedArgument pargs[1];
    if (!parseArgs(error, argv, argc, pargs, "s"))
        return -1;

    const char *srcstr = pargs[0].as_string.data;
    size_t      srclen = pargs[0].as_string.size;

    size_t offset = 0;
    size_t length = srclen;

    while (offset < srclen && srcstr[offset] == ' ')
        offset++;

    Heap *heap = Runtime_GetHeap(runtime);

    Object *result;
    if (offset == srclen)
        // The string only contained whitespace.
        // Return an empty string.
        result = Object_FromString("", 0, heap, error);
    else {
        
        // NOTE: The string doesn't contain only
        //       whitespace, so the length can't
        //       go under 1.
        while (srcstr[offset + length - 1] == ' ')
            length--;

        result = Object_FromString(srcstr + offset, length, heap, error);
    }

    if (result == NULL)
        return -1;
    rets[0] = result;
    return 1;
}

static int bin_toInt(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    // 0: string

    ParsedArgument pargs[1];
    if (!parseArgs(error, argv, argc, pargs, "s"))
        return -1;

    const char *srcstr = pargs[0].as_string.data;
    size_t      srclen = pargs[0].as_string.size;

    if (srclen == 0 || !isdigit(srcstr[0]))
        return returnValues2(error, runtime, rets, "ns", "String doesn't contain an integer");

    int64_t result = 0;
    size_t i = 0;
    do {
        int digit = srcstr[i] - '0';
        if (result > (INT64_MAX - digit) / 10)
            return returnValues2(error, runtime, rets, "ns", "An overflow occurred");
        result = result * 10 + digit;
        i++;
    } while (i < srclen && isdigit(srcstr[i]));

    if (i < srclen)
        // Found something other than a digit after the
        // sequence of digits, therefore the string can't
        // be considered an integer.
        return returnValues2(error, runtime, rets, "ns", "String doesn't contain an integer");
    
    return returnValues2(error, runtime, rets, "i", result);
}

StaticMapSlot bins_string[] = {
    { "ord",  SM_FUNCT, .as_funct = bin_ord, .argc = 1 },
    { "chr",  SM_FUNCT, .as_funct = bin_chr, .argc = 1 },
    { "cat",  SM_FUNCT, .as_funct = bin_cat, .argc = -1 },
    { "trim", SM_FUNCT, .as_funct = bin_trim, .argc = 1 },
    { "slice", SM_FUNCT, .as_funct = bin_slice, .argc = 3 },
    { "toInt", SM_FUNCT, .as_funct = bin_toInt, .argc = 1 },
};
