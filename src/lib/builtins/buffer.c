#include "buffer.h"
#include "utils.h"
#include "../utils/defs.h"
#include "../utils/utf8.h"
#include "../runtime.h"

static int bin_new(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    UNUSED(argc);
    ASSERT(argc == 1);

    if(!Object_IsInt(argv[0]))
    {
        Error_Report(error, ErrorType_RUNTIME, "Argument is not an int");
        return -1;
    }
    
    long long int size = Object_GetInt(argv[0]);

    Object *temp = Object_NewBuffer(size, Runtime_GetHeap(runtime), error);
    
    if(temp == NULL)
        return -1;
    
    rets[0] = temp;
    return 1;
}

static int bin_sliceUp(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    UNUSED(argc);
    ASSERT(argc == 3);

    ParsedArgument pargs[3];
    if (!parseArgs(error, argv, argc, pargs, "Bii"))
        return -1;
    long long int offset = pargs[1].as_int;
    long long int length = pargs[2].as_int;

    Object *temp = Object_SliceBuffer(argv[0], offset, length, Runtime_GetHeap(runtime), error);
    if(temp == NULL)
        return -1;

    rets[0] = temp;
    return 1;
}

static int bin_fromString(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    ASSERT(argc == 1);
    ParsedArgument pargs[1];
    if (!parseArgs(error, argv, argc, pargs, "s"))
        return -1;

    const char *str = pargs[0].as_string.data;
    size_t      len = pargs[0].as_string.size;
    Object *buffer = Object_NewBufferFromString(str, len, Runtime_GetHeap(runtime), error);
    if (buffer == NULL)
        return -1;
    rets[0] = buffer;
    return 1;
}

static int bin_toString(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    UNUSED(argc);
    ASSERT(argc == 1);

    if(!Object_IsBuffer(argv[0]))
    {
        Error_Report(error, ErrorType_RUNTIME, "Argument is not a buffer");
        return -1;
    }

    void  *buffaddr;
    size_t buffsize;

    buffaddr = Object_GetBuffer(argv[0], &buffsize);
    ASSERT(buffaddr != NULL);

    if(utf8_strlen(buffaddr, buffsize) < 0)
        return returnValues2(error, runtime, rets, "ns", "Buffer doesn't contain valid UTF-8");
    
    Object *temp = Object_FromString(buffaddr, buffsize, Runtime_GetHeap(runtime), error);

    if(temp == NULL)
        return -1;

    rets[0] = temp;
    return 1;
}

StaticMapSlot bins_buffer[] = {
    { "new",     SM_FUNCT, .as_funct = bin_new,     .argc = 1 },
    { "sliceUp", SM_FUNCT, .as_funct = bin_sliceUp, .argc = 3 },
    { "toString",   SM_FUNCT, .as_funct = bin_toString,   .argc = 1 },
    { "fromString", SM_FUNCT, .as_funct = bin_fromString, .argc = 1 },
};
