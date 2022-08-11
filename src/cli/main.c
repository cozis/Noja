
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../noja/noja.h"

static const char usage[] = 
	"Usage patterns:\n"
	"    $ noja run file.noja\n"
	"    $ noja run inline \"print('some noja code');\"\n"
	"    $ noja dis file.noja\n"
	"    $ noja dis inline \"print('some noja code');\"\n";

int main(int argc, char **argv)
{
	assert(argc > 0);

	if(argc == 1)
	{
		// $ noja
		fprintf(stderr, "Error: Incorrect usage.\n\n");
		fprintf(stderr, usage);
		return -1;
	}

	if(!strcmp(argv[1], "run"))
	{
		if(argc == 2)
		{
			fprintf(stderr, "Error: Missing source file.\n");
			return -1;
		}

		_Bool r;

		if(!strcmp(argv[2], "inline"))
		{
			if(argc == 3)
			{
				fprintf(stderr, "Error: Missing source string.\n");
				return -1;
			}
			r = NOJA_runString(argv[3]);
		}
		else
			r = NOJA_runFile(argv[2]);
		return r ? 0 : -1;
	}
	
	if(!strcmp(argv[1], "dis"))
	{
		if(argc == 2)
		{
			fprintf(stderr, "Error: Missing source file.\n");
			return -1;
		}

		_Bool r;

		if(!strcmp(argv[2], "inline"))
		{
			if(argc == 3)
			{
				fprintf(stderr, "Error: Missing source string.\n");
				return -1;
			}

			r = NOJA_dumpStringBytecode(argv[3]);
		}
		else
			r = NOJA_dumpFileBytecode(argv[2]);
		return r ? 0 : -1;
	}

	if(!strcmp(argv[1], "help"))
	{
		fprintf(stdout, usage);
		return 0;
	}

	fprintf(stderr, "Error: Incorrect usage.\n\n");
	fprintf(stderr, usage);
	return -1;
}