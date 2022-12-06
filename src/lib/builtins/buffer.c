#include "buffer.h"
#include "../utils/defs.h"
#include "../runtime/runtime.h"

static int bin_new(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    UNUSED(argc);
    ASSERT(argc == 1);

    if(!Object_IsInt(argv[0]))
    {
        Error_Report(error, 0, "Argument is not an int");
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

    if(!Object_IsInt(argv[1]))
    {
        Error_Report(error, 0, "Argument 1 is not an int");
        return -1;
    }

    if(!Object_IsInt(argv[2]))
    {
        Error_Report(error, 0, "Argument 2 is not an int");
        return -1;
    }

    long long int offset = Object_GetInt(argv[1]);
    long long int length = Object_GetInt(argv[2]);

    Object *temp = Object_SliceBuffer(argv[0], offset, length, Runtime_GetHeap(runtime), error);

    if(temp == NULL)
        return -1;

    rets[0] = temp;
    return 1;
}

static int bin_toString(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    UNUSED(argc);
    ASSERT(argc == 1);

    if(!Object_IsBuffer(argv[0]))
    {
        Error_Report(error, 0, "Argument is not a buffer");
        return -1;
    }

    void  *buffaddr;
    size_t buffsize;

    buffaddr = Object_GetBuffer(argv[0], &buffsize);
    ASSERT(buffaddr != NULL);

    Object *temp = Object_FromString(buffaddr, buffsize, Runtime_GetHeap(runtime), error);

    if(temp == NULL)
        return -1;

    rets[0] = temp;
    return 1;
}

StaticMapSlot bins_buffer[] = {
    { "new",   SM_FUNCT, .as_funct = bin_new, .argc = 1 },
    { "sliceUp", SM_FUNCT, .as_funct = bin_sliceUp, .argc = 3 },
    { "toString", SM_FUNCT, .as_funct = bin_toString, .argc = 1 },
};
