#include <assert.h>
#include "diagram.h"
#include "utils/source.h"
#include "utils/bpalloc.h"
#include "compiler/parse.h"
#include "compiler/graphviz.h"

static char *diagramSourceAST(Source *source, Error *error, 
							  int *error_offset)
{
	BPAlloc *alloc = BPAlloc_Init(-1);
    if(alloc == NULL)
    {
        *error_offset = -1;
        Error_Report(error, ErrorType_INTERNAL, "No memory");
        return NULL;
    }

    // NOTE: The AST is stored in the BPAlloc. Its
    //       lifetime is the same as the pool.
    AST *ast = parse(source, alloc, error, error_offset);
    if(ast == NULL)
    {
        assert(error->occurred);
        BPAlloc_Free(alloc);
        return NULL;
    }

    char *out = graphviz(ast, error, NULL);

    BPAlloc_Free(alloc);
    return out;
}

char *diagramFileAST(const char *file, Error *error, 
					 int *error_offset)
{
	Source *source = Source_FromFile(file, error);
	if (source == NULL)
		return NULL;

	char *out = diagramSourceAST(source, error, error_offset);

	Source_Free(source);
	return out;
}

char *diagramStringAST(const char *str, Error *error, 
					 int *error_offset)
{
	Source *source = Source_FromString(NULL, str, -1, error);
	if (source == NULL)
		return NULL;

	char *out = diagramSourceAST(source, error, error_offset);

	Source_Free(source);
	return out;
}