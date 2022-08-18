#include "buffer.h"
#include "../utils/defs.h"
#include "../runtime/runtime.h"

static int bin_new(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
    UNUSED(argc);
    ASSERT(argc == 1);

    long long int size = Object_ToInt(argv[0], error);

    if(error->occurred == 1)
        return -1;

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

    long long int offset = Object_ToInt(argv[1], error);
    if(error->occurred == 1) return -1;

    long long int length = Object_ToInt(argv[2], error);
    if(error->occurred == 1) return -1;

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

    void *buffaddr;
    int   buffsize;

    buffaddr = Object_GetBufferAddrAndSize(argv[0], &buffsize, error);

    if(error->occurred)
        return -1;

    Object *temp =  Object_FromString(buffaddr, buffsize, Runtime_GetHeap(runtime), error);

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
