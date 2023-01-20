#include <assert.h>
#include "../utils/bpalloc.h"
#include "AST.h"
#include "parse.h"
#include "codegen.h"
#include "compile.h"

Executable *compile(Source *src, Error *error, int *error_offset)
{
    // Create a bump-pointer allocator to hold the AST.
    BPAlloc *alloc = BPAlloc_Init(-1);

    if(alloc == NULL)
    {
        *error_offset = -1;
        Error_Report(error, ErrorType_INTERNAL, "No memory");
        return NULL;
    }

    // NOTE: The AST is stored in the BPAlloc. Its
    //       lifetime is the same as the pool.
    AST *ast = parse(src, alloc, error, error_offset);

    if(ast == NULL)
    {
        assert(error->occurred);
        BPAlloc_Free(alloc);
        return NULL;
    }
    
    // Transform the AST into bytecode.
    Executable *exe = codegen(ast, alloc, error, error_offset);

    // We're done with the AST.
    BPAlloc_Free(alloc);

    if(exe == NULL)
    {
        assert(error->occurred);
        return NULL;
    }

    return exe;
}