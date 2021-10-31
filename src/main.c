#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "compiler/parse.h"
#include "compiler/serialize.h"
#include "compiler/compile.h"
#include "runtime/runtime.h"
#include "runtime/runtime_error.h"

/* $ noja -f <file>
 * $ noja -c <text>
 * $ noja -f <file> -o <file>
 */

typedef enum { RUN, HELP, PARSE, VERSION, DISASSEMBLY } Action;

typedef struct {
	Action action;
	_Bool input_is_file, debug, verbose;
	const char *input, *output;
} Options;

static int parse_args(Options *opts, int argc, char **argv, Error *error)
{
	assert(argc >= 0);
	assert(opts != NULL);

	opts->action = RUN;
	opts->input_is_file = 1;
	opts->input  = NULL;
	opts->output = NULL;

	int i;

	for(i = 1; i < argc; i += 1)
		{
			if(!strcmp("-r", argv[i]) || !strcmp("--run", argv[i]))
				{
					opts->action = RUN;
				}
			else if(!strcmp("-p", argv[i]) || !strcmp("--parse", argv[i]))
				{
					opts->action = PARSE;
				}
			else if(!strcmp("-d", argv[i]) || !strcmp("--disassembly", argv[i]))
				{
					opts->action = DISASSEMBLY;
				}
			else if(!strcmp("-h", argv[i]) || !strcmp("--help", argv[i]))
				{
					opts->action = HELP;
				}
			else if(!strcmp("-v", argv[i]) || !strcmp("--version", argv[i]))
				{
					opts->action = VERSION;
				}
			else if(!strcmp("--debug", argv[i]))
				{
					opts->debug = 1;
				}
			else if(!strcmp("--verbose", argv[i]))
				{
					opts->verbose = 1;
				}
			else if(!strcmp("-f", argv[i]))
				{
					opts->input_is_file = 1;
				}
			else if(!strcmp("-c", argv[i]))
				{
					opts->input_is_file = 0;
				}
			else if(!strcmp("-o", argv[i]))
				{
					i += 1;

					if(i == argc)
						{
							Error_Report(error, 0, "Option -o requires a file name after it");
							return -1;
						}
					
					opts->output = argv[i];
				}
			else break;
		}

	if(i == argc)
		{
			Error_Report(error, 0, "Input file is missing");
			return -1;
		}

	opts->input = argv[i];

	i += 1;

	return i;
}

int main(int argc, char **argv)
{
	Error error;
	Error_Init(&error);


	// Parse command line options.

	Options opts;
	{
		int i = parse_args(&opts, argc, argv, &error);

		if(i < 0)
			{
				fprintf(stderr, "FATAL: %s.\n", error.message);
				Error_Free(&error);
				return 1;
			}
	}

	switch(opts.action)
		{
			case HELP:
			fprintf(stderr, "Not implemented yet.\n");
			return 1;

			case VERSION:
			fprintf(stderr, "Not implemented yet.\n");
			return 1;

			case RUN:
			{
				// Load
				Source *src;
				{
					if(opts.input_is_file)
						src = Source_FromFile(opts.input, &error);
					else
						src = Source_FromString(NULL, opts.input, -1, &error);

					if(src == NULL)
						{
							fprintf(stderr, "FATAL: %s.\n", error.message);
							return 1;
						}
				}

				// Compile
				Executable *exe;
				{
					BPAlloc *alloc = BPAlloc_Init(-1);

					if(alloc == NULL)
						return 1;

					AST *ast = parse(src, alloc, &error);
					
					if(ast == NULL)
						{
							fprintf(stderr, "PARSING ERROR: %s.\n", error.message);
							
							Error_Free(&error);
							BPAlloc_Free(alloc);
							Source_Free(src);
							return 1;
						}

					exe = compile(ast, alloc, &error);

					if(exe == NULL)
						{
							fprintf(stderr, "COMPILATION ERROR: %s.\n", error.message);
							
							Error_Free(&error);
							BPAlloc_Free(alloc);
							Source_Free(src);
							return 1;
						}

					BPAlloc_Free(alloc);
				}

				// Execute
				{
					Runtime *runtime = Runtime_New(-1, -1);
					
					if(runtime == NULL)
						{
							fprintf(stderr, "Couldn't initialize runtime.\n");
							Executable_Free(exe);
							Source_Free(src);
							return 1;
						}

					RuntimeError error;
					RuntimeError_Init(&error, runtime);

					Object *result = run(runtime, (Error*) &error, exe, 0, NULL, 0);

					if(result == NULL)
						{
							fprintf(stderr, "RUNTIME ERROR: %s.\n", error.base.message);
							
							if(error.snapshot)
								Snapshot_Print(error.snapshot, stderr);
							else
								fprintf(stderr, "No snapshot available.\n");

							Source_Free(src);
							Executable_Free(exe);
							RuntimeError_Free(&error);
							Runtime_Free(runtime);
							return 1;
						}
					else
						{
							if(Object_IsInt(result))
								{
									long long int val = Object_ToInt(result, (Error*) &error);
									assert(error.base.occurred == 0);

									fprintf(stderr, "%lld\n", val);
								}
							else if(Object_IsFloat(result))
								{
									double val = Object_ToFloat(result, (Error*) &error);
									assert(error.base.occurred == 0);

									fprintf(stderr, "%f\n", val);
								}
							else
								{
									fprintf(stderr, "Not printing returned value since it's not an int or a float.\n");
								}
						}

					Runtime_Free(runtime);
				}

				Executable_Free(exe);
				Source_Free(src);
				break;
			}

			case PARSE:
			{
				// Load
				Source *src;
				{
					if(opts.input_is_file)
						src = Source_FromFile(opts.input, &error);
					else
						src = Source_FromString(NULL, opts.input, -1, &error);

					if(src == NULL)
						{
							fprintf(stderr, "FATAL: %s.\n", error.message);
							return 1;
						}
				}

				BPAlloc *alloc = BPAlloc_Init(-1);

				if(alloc == NULL)
					return 1;


				Error error;
				Error_Init(&error);

				AST *ast = parse(src, alloc, &error);
				
				if(ast == NULL)
					{
						fprintf(stderr, "PARSING ERROR: %s.\n", error.message);
						
						Error_Free(&error);
						BPAlloc_Free(alloc);
						Source_Free(src);
						return 1;
					}
				
				int   len;
				char *str = serialize(ast, &len);

				_Bool failed = 0;

				if(str == NULL)
					{
						fprintf(stderr, "Failed to serialize\n");
						failed = 1;
					}
				else
					{
						if(opts.output == NULL)
							{
								fprintf(stdout, "%s\n", str);
							}
						else
							{
								FILE *fp = fopen(opts.output, "wb");

								if(fp == NULL)
									{
										fprintf(stderr, "Couldn't write to \"%s\".\n", opts.output);
										failed = 1;
									}
								else
									{
										int k = fwrite(str, 1, len, fp);

										if(k != len)
											{
												fprintf(stderr, "Only %d bytes out of %d could be written to \"%s\".\n", k, len, opts.output);
												failed = 1;
											}
									}
							}

						free(str);
					}

				Source_Free(src);
				BPAlloc_Free(alloc);
				return failed;
			}

			case DISASSEMBLY:
			fprintf(stderr, "Not implemented yet.\n");
			return 1;
		}
	return 0;
}

