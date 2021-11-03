#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "debug.h"
#include "utils/defs.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef enum {
	BY_LINE,
	BY_INDEX,
	BY_OFFSET,
} BreakPointMode;

typedef struct {
	BreakPointMode mode;
	char 		  *name;
	union {
		int line;
		int index;
		int offset;
	};
} BreakPoint;

struct xDebug {
	_Bool continuing;
	BreakPoint bpoints[32];
	int        bpoints_count;
};

Debug *Debug_New()
{
	Debug *dbg = malloc(sizeof(Debug));

	if(dbg == NULL)
		return NULL;

	dbg->continuing = 0;
	dbg->bpoints_count = 0;

	return dbg;
}

void Debug_Free(Debug *dbg)
{
	if(dbg)
		{
			for(int i = 0; i < dbg->bpoints_count; i += 1)
				free(dbg->bpoints[i].name);
			free(dbg);
		}
}

_Bool Debug_Callback(Runtime *runtime, void *userp)
{
	assert(runtime != NULL);
	assert(userp != NULL);

	Debug *dbg = (Debug*) userp;

	const char *name = NULL;
	int line = -1;
	int index = -1;
	int offset = -1;
	int length = -1;

	{
		Executable *exe = Runtime_GetCurrentExecutable(runtime);
		assert(exe != NULL);

		index = Runtime_GetCurrentIndex(runtime);
		assert(index >= 0);

		Source *src = Executable_GetSource(exe);

		if(src != NULL)
			{
				// Executable has source information.
				offset = Executable_GetInstrOffset(exe, index);
				assert(offset >= 0);

				length = Executable_GetInstrLength(exe, index);
				assert(length >= 0);

				name = Source_GetName(src);

				const char *body = Source_GetBody(src);
				assert(body != NULL);

				line = 1;

				int i = 0;
				
				while(i < offset)
					{
						if(body[i] == '\n')
							line += 1;

						i += 1;
					}

				assert(line > 0);
			}
	}

	// Check if we reached a breakpoint.
	
	_Bool hit = 0;
	int   idx;

	for(int i = 0; i < dbg->bpoints_count && hit == 0; i += 1)
		{
			if(!strcmp(dbg->bpoints[i].name, name))
				{
					switch(dbg->bpoints[i].mode)
						{
							case BY_LINE:
							if(dbg->bpoints[i].line == line)
								hit = 1;
							break;

							case BY_INDEX:
							if(dbg->bpoints[i].index == index)
								hit = 1;
							break;
							
							case BY_OFFSET:
							if(dbg->bpoints[i].offset >= offset && dbg->bpoints[i].offset < offset + length)
								hit = 1;
							break;
						}

					if(hit)
						idx = i;
				}
		}

	if(hit)
		{
			fprintf(stderr, "Hit breakpoint %d\n", idx);
			dbg->continuing = 0;
		}
	else
		{
			if(dbg->continuing)
				return 1;
		}


	char buffer[256];
	int  buflen;

	char *argv[32];
	int   argc;

	do {
		
		if(name == NULL && line == -1)
			fprintf(stderr, "(unnamed)" ANSI_COLOR_GREEN " > " ANSI_COLOR_RESET);
		else if(name == NULL && line > -1)
			fprintf(stderr, "(unnamed):%d" ANSI_COLOR_GREEN " > " ANSI_COLOR_RESET, line);
		else if(name != NULL && line == -1)
			fprintf(stderr, "%s" ANSI_COLOR_GREEN " > " ANSI_COLOR_RESET, name);
		else if(name != NULL && line > -1)
			fprintf(stderr, "%s:%d" ANSI_COLOR_GREEN " > " ANSI_COLOR_RESET, name, line);

		{
			buflen = 0;

			char c;
			while((c = getc(stdin)) != '\n')
				{
					if(buflen < (int) sizeof(buffer)-1)
						buffer[buflen] = c;
				
					buflen += 1;
				}

			if(buflen > (int) sizeof(buffer)-1)
				{
					fprintf(stdout, 
						"Command is too long. The internal buffer can only contain %ld bytes.\n"
						"Try inserting another command.\n", 
						sizeof(buffer)-1);
					continue;
				}

			buffer[buflen] = '\0';
		}

		// Now split the buffer into words in the (argc, argv) style.

		{
			argc = 0;

			_Bool again = 0;
			int curs = 0;
			while(1)
				{
					// Skip whitespace.
					while(isspace(buffer[curs]))
						curs += 1;

					if(buffer[curs] == '\0')
						break;

					int offset = curs;

					while(!isspace(buffer[curs]) && buffer[curs] != '\0')
						curs += 1;

					int length = curs - offset;

					assert(length > 0);

					{
						int max_argc = sizeof(argv) / sizeof(argv[0]);

						if(argc == max_argc)
							{
								fprintf(stdout, 
									"Command has too many words. The internal buffer can only contain %d words.\n"
									"Try inserting another command.\n", 
									max_argc);
								again = 1;
								break;
							}

						argv[argc++] = buffer + offset;
					}

					if(buffer[curs] == '\0')
						break;

					assert(isspace(buffer[curs]));

					buffer[curs] = '\0';

					curs += 1; // Consume the space that ended the word (not overwritten by a zero byte).
				}

			if(again)
				continue;
		}

		if(argc == 0)
			continue;

		{
			if(!strcmp(argv[0], "help"))
				{
					if(argc == 1)
						{
					fprintf(stderr, 
						"help .............. Show this message\n"
						"help <command> .... Display additional information for a given command\n"
						"step .............. Run an instruction\n"
						"quit .............. Stop execution\n"
						"continue .......... Run until a breakpoint or the end of the code is reached\n"
						"breakpoint ........ Add a breakpoint\n"
						"stack ............. Show the contents of the stack\n");
						}
					else if(!strcmp(argv[1], "help"))
						{
							fprintf(stderr, 
								"\n"
								"     Command | help\n"
								"             | \n"
								"       Usage | > help [<command>]\n"
								"             | \n"
								" Description | List all available commands or show detailed \n"
								"             | information about a given command.\n"
								"\n");
						}
					else if(!strcmp(argv[1], "step"))
						{
							fprintf(stderr, 
								"\n"
								"     Command | step\n"
								"             | \n"
								"       Usage | > step\n"
								"             | \n"
								" Description | Run a single instruction.\n"
								"\n");
						}
					else if(!strcmp(argv[1], "quit"))
						{
							fprintf(stderr, 
								"\n"
								"     Command | quit\n"
								"             | \n"
								"       Usage | > quit\n"
								"             | \n"
								" Description | Stop the execution.\n"
								"\n");
						}
					else if(!strcmp(argv[1], "continue"))
						{
							fprintf(stderr, 
								"\n"
								"     Command | continue\n"
								"             | \n"
								"       Usage | > continue\n"
								"             | \n"
								" Description | Run until a breakpoint or the end of the code is\n"
								"             | reached.\n"
								"\n");
						}
					else if(!strcmp(argv[1], "breakpoint"))
						{
							fprintf(stderr, 
								"\n"
								"     Command | breakpoint\n"
								"             | \n"
								"       Usage | > breakpoint <file> { line | offset | index } <N>\n"
								"             | \n"
								" Description | Add a breakpoint. The first argument of the command \n"
								"             | specifies the source containing the breakpoint, while \n"
								"             | the second one specifies the way the breakpoint is \n"
								"             | expressed: \n"
								"             | \n"
								"             |       line  | by line number.\n"
								"             |             | \n"
								"             |     offset  | by character offset.\n"
								"             |             | \n"
								"             |      index  | by instruction index (relative to the \n"
								"             |             | start of the source executable's body).\n"
								"             | \n"
								"             | the third argument is, based on the second one, either\n"
								"             | a line number, a character offset or an instruction index.\n"
								"\n");
						}
					else if(!strcmp(argv[1], "stack"))
						{
							fprintf(stderr, 
								"\n"
								"     Command | stack\n"
								"             | \n"
								"       Usage | > stack\n"
								"             | \n"
								" Description | Show the contents of the stack.\n"
								"\n");
						}
					else
						{
							fprintf(stdout, "Unknown command \"%s\".\n", argv[1]);
						}
				}
			else if(!strcmp(argv[0], "breakpoint"))
				{
					if(argc < 3)
						{
							fprintf(stderr, "\"breakpoint\" command expects 2 operands.\n");
							continue;
						}

					int max_breakpoints = sizeof(dbg->bpoints) / sizeof(dbg->bpoints[0]);

					if(dbg->bpoints_count == max_breakpoints)
						{
							fprintf(stderr, "Can't add a breakpoint. The maximum number of breakpoints (%d) was reached.\n", max_breakpoints);
						}
					else
						{
							// Handle second argument.

							BreakPointMode mode;

							if(!strcmp(argv[2], "line"))
								{
									mode = BY_LINE;
								}
							else if(!strcmp(argv[2], "index"))
								{
									mode = BY_INDEX;
								}
							else if(!strcmp(argv[2], "offset"))
								{
									mode = BY_OFFSET;
								}
							else
								{
									fprintf(stderr, "Bad second argument \"%s\". It must be either one of \"line\", \"index\", \"offset\".\n", argv[2]);
									continue;
								}

							// Handle third argument.

							long long int N = strtoll(argv[3], NULL, 10);

							if(errno == ERANGE)
								{
									if(N == LLONG_MIN)
										{
											fprintf(stderr, "Bad third argument. Integer is too big.\n");
										}
									else
										{
											assert(N == LLONG_MAX);
											fprintf(stderr, "Bad third argument. Integer is too big.\n");
										}
									continue;
								}
							else if(errno != 0)
								{
									fprintf(stderr, "Bad third argument. (strtoll says: %s).\n", strerror(errno));
									continue;
								}

							// Handle first argument.

							char *name_copy = malloc(strlen(argv[1])+1);

							if(name_copy == NULL)
								{
									fprintf(stderr, "Uoops! No memory!");
									continue;
								}

							strcpy(name_copy, argv[1]);

							dbg->bpoints[dbg->bpoints_count] = (BreakPoint) { 
								.mode = mode, 
								.name = name_copy,
								.line = N,
							};

							dbg->bpoints_count += 1;
							fprintf(stderr, "BreakPoint added.\n");
						}
				}
			else if(!strcmp(argv[0], "continue"))
				{
					dbg->continuing = 1;
					return 1;
				}
			else if(!strcmp(argv[0], "step"))
				{
					return 1;
				}
			else if(!strcmp(argv[0], "stack"))
				{
					Stack *stack = Runtime_GetStack(runtime);
					assert(stack != NULL);
					
					if(Stack_Size(stack) == 0)
						fprintf(stderr, "The stack is empty.\n");

					for(int i = 0; i < (int) Stack_Size(stack); i += 1)
						{
							Object *obj = Stack_Top(stack, -i);
							assert(obj != NULL);

							fprintf(stderr, "  %d | ", i);
							Object_Print(obj, stderr);
							fprintf(stderr, "\n");
						}
				}
			else if(!strcmp(argv[0], "quit"))
				{
					return 0;
				}
			else
				{
					fprintf(stdout, "Unknown command \"%s\". Type \"help\" is you need help.\n", argv[0]);
				}
		}

	} while(1);

	UNREACHABLE;
	return 0;
}
