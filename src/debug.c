#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "debug.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef struct {
	char *name;

} BreakPoint;

struct xDebug {

};

Debug *Debug_New()
{
	Debug *dbg = malloc(sizeof(Debug));

	if(dbg == NULL)
		return NULL;

	// Do stuff here.

	return dbg;
}

void Debug_Free(Debug *dbg)
{
	if(dbg)
		free(dbg);
}

_Bool Debug_Callback(Runtime *runtime, void *userp)
{
	assert(runtime != NULL);
	assert(userp != NULL);

	Debug *dbg = (Debug*) userp;

	char buffer[256];
	int  length;

	int argc;
	char *argv[32];

	do {
		
		fprintf(stderr, ANSI_COLOR_GREEN "> " ANSI_COLOR_RESET);

		{
			length = 0;

			char c;
			while((c = getc(stdin)) != '\n')
				{
					if(length < (int) sizeof(buffer)-1)
						buffer[length] = c;
				
					length += 1;
				}

			if(length > (int) sizeof(buffer)-1)
				{
					fprintf(stdout, 
						"Command is too long. The internal buffer can only contain %ld bytes.\n"
						"Try inserting another command.\n", 
						sizeof(buffer)-1);
					continue;
				}

			buffer[length] = '\0';
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
						"breakpoint ........ Add a breakpoint\n");
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
					else
						{
							fprintf(stdout, "Unknown command \"%s\".\n", argv[1]);
						}
				}
			else if(!strcmp(argv[0], "breakpoint"))
				{
					fprintf(stderr, "Not implemented yet.\n");
				}
			else if(!strcmp(argv[0], "continue"))
				{
					fprintf(stderr, "Not implemented yet.\n");
				}
			else if(!strcmp(argv[0], "step"))
				{
					break;
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

	return 1;
}
