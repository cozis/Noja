#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <xjson.h>
#include "../../src/compiler/parse.h"
#include "../../src/compiler/serialize.h"

typedef enum { PASSED, FAILED, IGNORED } result_t;

static _Bool compare(const char *s, xj_value *v)
{
	xj_alloc *alloc = NULL;
	xj_error  error;
	xj_value *doc;

	doc = xj_decode(&error, &alloc, s, -1);
	assert(doc != NULL);

	_Bool same = xj_compare(doc, v);

	xj_free(alloc);
	return same;
}

result_t runtest(xj_value *value, int testno)
{
	int 	  raw_sourcel;
	char 	 *raw_source;
	xj_value *expect;

	{
		xj_error error;
		xj_value *source2 = xj_query(&error, value, ".source", -1);

		if(source2 == NULL)
			{
				if(error.code == xj_E4009)
					fprintf(stderr, "WARNING: Field \"source\" is missing\n");
				else
					fprintf(stderr, "WARNING: Internal failure (%s)\n", error.text);
				return IGNORED;
			}

		if(source2->type != xj_STRING)
			{
				fprintf(stderr, "WARNING: Field \"source\" is not a string\n");
				return IGNORED;
			}

		raw_source  = source2->as_string;
		raw_sourcel = source2->size;

		expect = xj_query(&error, value, ".expect", -1);

		if(expect == NULL)
			{
				if(error.code == xj_E4009)
					fprintf(stderr, "WARNING: Field \"expect\" is missing\n");
				else
					fprintf(stderr, "WARNING: Internal failure (%s)\n", error.text);
				return IGNORED;
			}

		if(expect->type != xj_OBJECT)
			{
				fprintf(stderr, "WARNING: Field \"expect\" is not an object\n");
				return IGNORED;
			}
	}

	Source *source;
	{
		Error error;
		Error_Init(&error);

		char temp[64];
		int k = snprintf(temp, sizeof(temp), "(Test %d's source)", testno);

		assert(k >= 0);
		assert((unsigned int) k < sizeof(temp));

		source = Source_FromString(temp, raw_source, raw_sourcel, &error);
		
		if(source == NULL)
			{
				fprintf(stderr, "WARNING: Internal failure (%s)\n", error.message);
				return IGNORED;
			}
	}

	_Bool passed;
	{
		Error 	 error;
		BPAlloc *alloc;
		char 	 buffer[65536];

		Error_Init(&error);

		alloc = BPAlloc_Init3(buffer, sizeof(buffer), -1, NULL, NULL, NULL);

		if(alloc == NULL)
			{
				fprintf(stderr, "WARNING: Internal failure (Failed to instanciate BPAlloc)\n");
				return IGNORED;
			}

		AST *ast = parse(source, alloc, &error);

		if(ast == NULL)
			{
				fprintf(stderr, "WARNING: Internal failure (Failed to compile: %s; reported in %s at %s:%d)\n", error.message, error.func, error.file, error.line);
				
				Error_Free(&error);
				BPAlloc_Free(alloc);
				return IGNORED;
			}

		char *serialized = serialize(ast, NULL);
		
		if(serialized == NULL)
			{
				fprintf(stderr, "WARNING: Internal failure (Failed to serialize AST)\n");
				
				Error_Free(&error);
				BPAlloc_Free(alloc);
				return IGNORED;
			}

		/*
		printf("SERIALIZED   :: %s\n", serialized);
		{
			char *temp = xj_encode(expect, NULL);
			printf("SERIALIZED 2 :: %s\n", temp);
			xj_sys_free(temp);
		}
		*/

		passed = compare(serialized, expect);

		Error_Free(&error);
		BPAlloc_Free(alloc);
		xj_sys_free(serialized);
	}

	return passed ? PASSED : FAILED;
}

static char *loadfile(const char *file, int *size);

int main(int argc, char **argv)
{
	if(argc == 1)
		{
			fprintf(stderr, "ERROR: Missing file name\n");
			return 1;
		}

	int   filesize;
	char *filename = argv[1];
	char *filetext = loadfile(filename, &filesize);

	if(filetext == NULL)
		{
			fprintf(stderr, "ERROR: Couldn't open \"%s\"\n", filename);
			return 1;
		}

	xj_alloc *alloc = NULL;
	xj_error  error;
	xj_value *document;
	
	document = xj_decode(&error, &alloc, filetext, filesize);

	free(filetext);

	if(document == NULL)
		{
			assert(error.occurred == 1);
			fprintf(stderr, "ERROR: Failed to parse \"%s\" as JSON: %s\n", 
				filename, error.text);
			xj_free(alloc);
			return 1;
		}

	if(document->type != xj_ARRAY)
		{
			fprintf(stderr, "ERROR: Was expected an array as JSON document root\n");
			xj_free(alloc);
			return 1;
		}

	xj_value *cursor = document->as_array;

	int passed = 0, ignored = 0;
	int total = document->size;
	int index = 1;

	while(cursor)
		{
			result_t result = runtest(cursor, index);

			#define KNRM  "\x1B[0m"
			#define KRED  "\x1B[31m"
			#define KGRN  "\x1B[32m"
			#define KYEL  "\x1B[33m"
			#define KBLU  "\x1B[34m"
			#define KMAG  "\x1B[35m"
			#define KCYN  "\x1B[36m"
			#define KWHT  "\x1B[37m"
			#define RESET "\033[0m"

			switch(result)
				{
					case PASSED:
					printf("TEST %-2d :: " KGRN "PASSED" RESET "\n", index); 
					passed += 1; 
					break;
					
					case FAILED: 
					printf("TEST %-2d :: " KRED "FAILED" RESET "\n", index); 
					break;
					
					case IGNORED: 
					printf("TEST %-2d :: " KYEL "IGNORED" RESET "\n", index); 
					ignored += 1;
					break;
					
					default: assert(0);
				}

			index += 1;
			cursor = cursor->next;
		}

	printf("\n  ... %d passed, %d failed and %d ignored.\n", passed, total - passed - ignored, ignored);

	xj_free(alloc);
	return 0;
}

static char *loadfile(const char *file, int *size)
{
	char *body = NULL;
	int _size;

	FILE *fp = fopen(file, "rb");

	if(fp == NULL)
		return NULL;

	if(fseek(fp, 0, SEEK_END))
		goto done;

	_size = ftell(fp);

	if(_size < 0)
		goto done;

	if(size)
		*size = _size;

	body = malloc(_size + 1);

	if(body == NULL)
		goto done;

	if(fseek(fp, 0, SEEK_SET))
		goto done;

	int k = fread(body, 1, _size, fp);

	if(k != _size)
		goto done;

	body[_size] = '\0';

done:
	fclose(fp);
	return body;
}