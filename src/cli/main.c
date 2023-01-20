
/* +--------------------------------------------------------------------------+
** |                          _   _       _                                   |
** |                         | \ | |     (_)                                  |
** |                         |  \| | ___  _  __ _                             |
** |                         | . ` |/ _ \| |/ _` |                            |
** |                         | |\  | (_) | | (_| |                            |
** |                         |_| \_|\___/| |\__,_|                            |
** |                                    _/ |                                  |
** |                                   |__/                                   |
** +--------------------------------------------------------------------------+
** | Copyright (c) 2022 Francesco Cozzuto <francesco.cozzuto@gmail.com>       |
** +--------------------------------------------------------------------------+
** | This file is part of The Noja Interpreter.                               |
** |                                                                          |
** | The Noja Interpreter is free software: you can redistribute it and/or    |
** | modify it under the terms of the GNU General Public License as published |
** | by the Free Software Foundation, either version 3 of the License, or (at |
** | your option) any later version.                                          |
** |                                                                          |
** | The Noja Interpreter is distributed in the hope that it will be useful,  |
** | but WITHOUT ANY WARRANTY; without even the implied warranty of           |
** | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General |
** | Public License for more details.                                         |
** |                                                                          |
** | You should have received a copy of the GNU General Public License along  |
** | with The Noja Interpreter. If not, see <http://www.gnu.org/licenses/>.   |
** +--------------------------------------------------------------------------+ 
*/
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include "../lib/run.h"
#include "../lib/runtime.h"

static void usage(FILE *stream, const char *name) 
{
	fprintf(stream, 
		"USAGE\n"
		"  $ %s [-h | -o <file> | -p | -H <heap size> | {-d | -a}] [--] <file>\n", name);
}

static void help(FILE *stream, const char *name)
{
	usage(stream, name);
	fprintf(stream, 
		"OPTIONS\n"
		"  -h, --help        Show this message\n"
		"  -d, --disassembly Output the bytecode associated to noja code\n"
		"  -i, --inline      Execute a string of code instead of a file\n"
		"  -a, --assembly    Specify that the source is bytecode and not noja code\n"
		"  -p, --profile     Profile the execution of the source (can't be used with -d)\n"
		"  -o, --output      Specify the output file of -p or -d\n"
		"  -H, --heap <size> Heap size\n"
		"\n");
}

typedef enum {
	Mode_DISASSEMBLY,
	Mode_ASSEMBLY,
	Mode_DEFAULT,
	Mode_HELP,
} Mode;

Runtime *runtime = NULL;

static void signalHandler(int signo)
{
    (void) signo;
    if (runtime != NULL)
       Runtime_Interrupt(runtime);
}

int main(int argc, char **argv)
{
	Mode mode = Mode_DEFAULT;
	bool profile = false;
	bool no_file = false;
	const char *output = NULL;
	const char *input  = NULL;
	size_t heap = 1024 * 1024;

	for (int i = 1; i < argc; i++) {

		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
	
			mode = Mode_HELP;
		
		} else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--disassembly")) {

			mode = Mode_DISASSEMBLY;

		} else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--inline")) {
			
			no_file = true;

		} else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--assembly")) {

			mode = Mode_ASSEMBLY;

		} else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--profile")) {

			profile = true;

		} else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) {

			if (i+1 == argc || argv[i+1][0] == '-') {
				fprintf(stderr, "Missing file path after %s option\n", argv[i]);
				usage(stderr, argv[0]);
				return -1;
			}
			output = argv[++i];

		} else if (!strcmp(argv[i], "-H") || !strcmp(argv[i], "--heap")) {

			if (i+1 == argc || argv[i+1][0] == '-') {
				fprintf(stderr, "Missing byte count after %s option\n", argv[i]);
				usage(stderr, argv[0]);
				return -1;
			}
			heap = atoi(argv[++i]);
			if (heap == 0) {
				fprintf(stderr, "Invalid heap size\n");
				usage(stderr, argv[0]);
				return -1;
			}
			
		} else {
			input = argv[i];
			break;
		}
	}

	int code;
	switch (mode) {
		
		case Mode_HELP: 
		help(stdout, argv[0]); 
		code = 0; 
		break;
		
		case Mode_DEFAULT:
		case Mode_ASSEMBLY:
		{
			if (input == NULL) {
				fprintf(stderr, "No input file");
				code = -1;
				break;
			}
			
			RuntimeConfig config = Runtime_GetDefaultConfigs();
			config.time = profile;
			config.heap = heap;

			runtime = Runtime_New(config);
			if (runtime == NULL) {
				fprintf(stderr, "Failed to initialize runtime");
				code = -1;
				break;
			}
			signal(SIGINT,  signalHandler);
		   signal(SIGTERM, signalHandler);

			Error error;
		   Error_Init(&error);

		   if (!Runtime_plugDefaultBuiltins(runtime, (Error*) &error)) {
		   	Error_Print(&error, ErrorType_RUNTIME, stderr);
		   	Error_Free(&error);
		   	Runtime_PrintStackTrace(runtime, stderr);
		    	Runtime_Free(runtime);
		    	code = -1;
		    	break;
		   }

		   bool ok;
		   if (mode == Mode_ASSEMBLY) {
		    	if (no_file)
			    	ok = runBytecodeString(runtime, input, (Error*) &error);
			   else
					ok = runBytecodeFile(runtime, input, (Error*) &error);
		   } else {
			   if (no_file)
				  	ok = runString(runtime, input, (Error*) &error);
			   else
					ok = runFile(runtime, input, (Error*) &error);
			}

			if (ok == false) {
				Error_Print(&error, ErrorType_RUNTIME, stderr);
		    	Error_Free(&error);
		    	Runtime_PrintStackTrace(runtime, stderr);
		    	Runtime_Free(runtime);
		    	code = -1;
		    	break;
			}
			
			code = 0;
			if (output == NULL)
				Runtime_SerializeProfilingResultsToStream(runtime, stdout);
			else
				Runtime_SerializeProfilingResultsToFile(runtime, output);
			Runtime_Free(runtime);
			break;
		}
		
		case Mode_DISASSEMBLY:
		if (input == NULL) {
			fprintf(stderr, "No disassembly input file");
			code = -1;
			break;
		}
		/* .. */
		break;
	}

	return code;
}

/*
	noja [-o <file> | -p | {-d | -a}] [--] <file>
	
	-o can only be used with -d or -a
	
	-h --help
	-d --disassembly disassembly
	-i --inline      inline
	-a --assembly
    -p --profile
	-o --output
	
    noja <file>
    noja -i <code>

*/