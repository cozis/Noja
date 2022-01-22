#include <string.h>
#include "compiler/parse.h"
#include "compiler/compile.h"
#include "runtime/runtime.h"
#include "o_builtins.h"
#include "eval.h"

Object *eval(const char *str, int len, Object *closure, Heap *heap, Error *error)
{
	if(len < 0)
		len = strlen(str);

	Source *s = Source_FromString(NULL, str, len, error);

	if(s == NULL)
		return NULL; // Propagate.

	BPAlloc *alloc = BPAlloc_Init(-1);

	if(alloc == NULL)
		{
			Error_Report(error, 1, "Insufficient memory");
			return NULL;
		}

	AST *ast = parse(s, alloc, error);

	if(ast == NULL)
		{
			BPAlloc_Free(alloc);
			Source_Free(s);
			return NULL;
		}

	Executable *exe = compile(ast, alloc, error);

	if(exe == NULL)
		{
			BPAlloc_Free(alloc);
			Source_Free(s);
			return NULL;
		}

	BPAlloc_Free(alloc);
	Source_Free(s);

	Runtime *runt = Runtime_New2(-1, heap, 0, NULL, NULL);

	if(runt == NULL)
		{
			Error_Report(error, 1, "Insufficient memory");
			Executable_Free(exe);
			return NULL;
		}

	Object *bins = Object_NewBuiltinsMap(runt, heap, error);

	if(bins == NULL)
		{
			Runtime_Free(runt);
			Executable_Free(exe);
			return NULL;
		}

	Runtime_SetBuiltins(runt, bins);

	Object *o = run(runt, error, exe, 0, closure, NULL, 0);

	if(o == NULL)
		{
			Runtime_Free(runt);
			Executable_Free(exe);
			return NULL;
		}

	Runtime_Free(runt);
	Executable_Free(exe);
	return o;
}