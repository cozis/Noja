#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "o_staticmap.h"
#include "compiler/parse.h"
#include "compiler/serialize.h"
#include "compiler/compile.h"
#include "runtime/runtime.h"
#include "runtime/runtime_error.h"
#include "builtins/basic.h"

static const char usage[] = 
	"Usage patterns:\n"
	"    $ noja run file.noja\n"
	"    $ noja run inline \"print('some noja code');\"\n"
	"    $ noja dis file.noja\n"
	"    $ noja dis inline \"print('some noja code');\"\n";
static _Bool interpret_file(const char *file);
static _Bool interpret_code(const char *code);
static _Bool disassemble_file(const char *file);
static _Bool disassemble_code(const char *code);

static void print_error(const char *type, Error *error)
{
	if(type == NULL)
		fprintf(stderr, "Error");
	else if(error->internal)
		fprintf(stderr, "Internal Error");
	else
		fprintf(stderr, "%s Error", type);

	fprintf(stderr, ": %s. ", error->message);

	if(error->file != NULL)
		{
			if(error->line > 0 && error->func != NULL)
				fprintf(stderr, "(Reported in %s:%d in %s)", error->file, error->line, error->func);
			else if(error->line > 0 && error->func == NULL)
				fprintf(stderr, "(Reported in %s:%d)", error->file, error->line);
			else if(error->line < 1 && error->func != NULL)
				fprintf(stderr, "(Reported in %s in %s)", error->file, error->func);
		}

	fprintf(stderr, "\n");
}

_Bool noja(int argc, char **argv)
{
	assert(argc > 0);

	if(argc == 1)
		{
			// $ noja
			fprintf(stderr, "Error: Incorrect usage.\n\n");
			fprintf(stderr, usage);
			return 0;
		}

	if(!strcmp(argv[1], "run"))
		{
			Error error;
			Error_Init(&error);
			
			if(argc == 2)
				{
					Error_Report(&error, 0, "Missing source file");
					print_error(NULL, &error);
					Error_Free(&error);
					return 0;
				}

			_Bool r;

			if(!strcmp(argv[2], "inline"))
				{
					if(argc == 3)
						{
							Error_Report(&error, 0, "Missing source string");
							print_error(NULL, &error);
							Error_Free(&error);
							return 0;
						}

					r = interpret_code(argv[3]);
				}
			else
				r = interpret_file(argv[2]);
			return r;
		}
	
	if(!strcmp(argv[1], "dis"))
		{
			Error error;
			Error_Init(&error);
			
			if(argc == 2)
				{
					Error_Report(&error, 0, "Missing source file");
					print_error(NULL, &error);
					Error_Free(&error);
					return 0;
				}

			_Bool r;

			if(!strcmp(argv[2], "inline"))
				{
					if(argc == 3)
						{
							Error_Report(&error, 0, "Missing source string");
							print_error(NULL, &error);
							Error_Free(&error);
							return 0;
						}

					r = disassemble_code(argv[3]);
				}
			else
				r = disassemble_file(argv[2]);
			return r;
		}

	if(!strcmp(argv[1], "help"))
		{
			fprintf(stdout, usage);
			return 1;
		}

	fprintf(stderr, "Error: Incorrect usage.\n\n");
	fprintf(stderr, usage);
	return 0;
}

static _Bool interpret(Source *src)
{
	// Compile the code. This section transforms
	// a [Source] into an [Executable].
	Executable *exe;
	{
		// Create a bump-pointer allocator to hold the AST.
		BPAlloc *alloc = BPAlloc_Init(-1);

		if(alloc == NULL)
			{
				fprintf(stderr, "Internal Error: Couldn't allocate bump-pointer allocator to hold the AST.\n");
				return 0;
			}

		Error error;
		Error_Init(&error);

		// NOTE: The AST is stored in the BPAlloc. It's
		//       lifetime is the same as the pool.
		AST *ast = parse(src, alloc, &error);
	
		if(ast == NULL)
			{
				assert(error.occurred);
				print_error("Parsing", &error);
				Error_Free(&error);
				BPAlloc_Free(alloc);
				return 0;
			}

		exe = compile(ast, alloc, &error);

		// We're done with the AST, independently from
		// the compilation result.
		BPAlloc_Free(alloc);

		if(exe == NULL)
			{
				assert(error.occurred);
				print_error("Compilation", &error);
				Error_Free(&error);
				return 0;
			}
	}

	// Now execute it.
	{
		Runtime *runt = Runtime_New(-1, -1, NULL, NULL);

		if(runt == NULL)
			{
				Error error;
				Error_Init(&error);
				Error_Report(&error, 1, "Couldn't initialize runtime");
				print_error(NULL, &error);
				Error_Free(&error);
				Executable_Free(exe);
				return 0;
			}

		// We use a [RuntimeError] instead of a simple [Error]
		// because the [RuntimeError] makes a snapshot of the
		// runtime state when an error is reported. Other than
		// this fact they are interchangable. Any function that
		// expects a pointer to [Error] can receive a [RuntimeError]
		// upcasted to [Error].
		RuntimeError error;
		RuntimeError_Init(&error, runt); // Here we specify the runtime to snapshot in case of failure.
		
		Object *bins = Object_NewStaticMap(bins_basic, runt, (Error*) &error);

		if(bins == NULL)
			{
				assert(error.base.occurred == 1);
				print_error(NULL, (Error*) &error);
				RuntimeError_Free(&error);
				Executable_Free(exe);
				Runtime_Free(runt);
				return 0;
			}

		Runtime_SetBuiltins(runt, bins);

		Object *o = run(runt, (Error*) &error, exe, 0, NULL, NULL, 0);

		// NOTE: The pointer to the builtins object is invalidated
		//       now because it may be moved by the garbage collector.

		if(o == NULL)
			{
				print_error("Runtime", (Error*) &error);

				if(error.snapshot == NULL)
					fprintf(stderr, "No snapshot available.\n");
				else
					Snapshot_Print(error.snapshot, stderr);

				RuntimeError_Free(&error);
			}

		Runtime_Free(runt);
		Executable_Free(exe);

		return o != NULL;
	}
}

static _Bool disassemble(Source *src)
{
	// Compile the code. This section transforms
	// a [Source] into an [Executable].
	Executable *exe;
	{
		// Create a bump-pointer allocator to hold the AST.
		BPAlloc *alloc = BPAlloc_Init(-1);

		if(alloc == NULL)
			{
				fprintf(stderr, "Internal Error: Couldn't allocate bump-pointer allocator to hold the AST.\n");
				return 0;
			}

		Error error;
		Error_Init(&error);

		// NOTE: The AST is stored in the BPAlloc. It's
		//       lifetime is the same as the pool.
		AST *ast = parse(src, alloc, &error);
	
		if(ast == NULL)
			{
				assert(error.occurred);
				print_error("Parsing", &error);
				Error_Free(&error);
				BPAlloc_Free(alloc);
				return 0;
			}

		exe = compile(ast, alloc, &error);

		// We're done with the AST, independently from
		// the compilation result.
		BPAlloc_Free(alloc);

		if(exe == NULL)
			{
				assert(error.occurred);
				print_error("Compilation", &error);
				Error_Free(&error);
				return 0;
			}
	}

	Executable_Dump(exe);
	return 1;
}

static _Bool interpret_file(const char *file)
{
	Error error;
	Error_Init(&error);
	
	Source *src = Source_FromFile(file, &error);

	if(src == NULL)
		{
			assert(error.occurred == 1);
			print_error(NULL, &error);
			Error_Free(&error);
			return 0;
		}

	_Bool r = interpret(src);

	Source_Free(src);
	return r;
}

static _Bool interpret_code(const char *code)
{
	Error error;
	Error_Init(&error);
	
	Source *src = Source_FromString(NULL, code, -1, &error);

	if(src == NULL)
		{
			assert(error.occurred);
			print_error(NULL, &error);
			Error_Free(&error);
			return 0;
		}

	_Bool r = interpret(src);

	Source_Free(src);
	return r;
}

static _Bool disassemble_file(const char *file)
{
	Error error;
	Error_Init(&error);
	
	Source *src = Source_FromFile(file, &error);

	if(src == NULL)
		{
			assert(error.occurred == 1);
			print_error(NULL, &error);
			Error_Free(&error);
			return 0;
		}

	_Bool r = disassemble(src);

	Source_Free(src);
	return r;
}

static _Bool disassemble_code(const char *code)
{
	Error error;
	Error_Init(&error);
	
	Source *src = Source_FromString(NULL, code, -1, &error);

	if(src == NULL)
		{
			assert(error.occurred);
			print_error(NULL, &error);
			Error_Free(&error);
			return 0;
		}

	_Bool r = disassemble(src);

	Source_Free(src);
	return r;
}