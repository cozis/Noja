#include <assert.h>
#include "../utils/bpalloc.h"
#include "AST.h"
#include "parse.h"
#include "codegen.h"
#include "compile.h"

Executable *compile(Source *src, Error *error, CompilationErrorType *errtyp)
{   
    // Create a bump-pointer allocator to hold the AST.
    BPAlloc *alloc = BPAlloc_Init(-1);

    if(alloc == NULL)
    {
        *errtyp = CompilationErrorType_INTERNAL;
        Error_Report(error, 1, "No memory");
        return NULL;
    }

    // NOTE: The AST is stored in the BPAlloc. It's
    //       lifetime is the same as the pool.
    AST *ast = parse(src, alloc, error);

    if(ast == NULL)
    {
        assert(error->occurred);
        if(error->internal)
            *errtyp = CompilationErrorType_INTERNAL;
        else
            *errtyp = CompilationErrorType_SYNTAX;
        BPAlloc_Free(alloc);
        return NULL;
    }
    
    // Transform the AST into bytecode.
    Executable *exe = codegen(ast, alloc, error);

    // We're done with the AST.
    BPAlloc_Free(alloc);

    if(exe == NULL)
    {
        assert(error->occurred);
        if(error->internal)
            *errtyp = CompilationErrorType_INTERNAL;
        else
            *errtyp = CompilationErrorType_SEMANTIC;
        return NULL;
    }

    return exe;
}