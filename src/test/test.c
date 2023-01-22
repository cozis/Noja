#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "error.h"
#include "field.h"

static bool growFieldsArray(NojaTest *test)
{
	size_t new_capacity = 2 * test->fields_capacity;

	NojaTestField *new_fields;
	if (test->fields == test->fields_maybe) {
		new_fields = malloc(new_capacity * sizeof(NojaTestField));
		if (new_fields != NULL)
			memcpy(new_fields, test->fields, test->fields_count * sizeof(NojaTestField));
	} else
		new_fields = realloc(test->fields, new_capacity * sizeof(NojaTestField));

	if (new_fields == NULL)
		return false;

	test->fields = new_fields;
	test->fields_capacity = new_capacity;
	return true;
}

void freeTest(NojaTest *test)
{
	for (size_t i = 0; i < test->fields_count; i++)
		freeField(test->fields + i);
	if (test->fields != test->fields_maybe)
		free(test->fields);
}

const char *queryTest(NojaTest *test, const char *name)
{
	for (size_t i = 0; i < test->fields_count; i++)
		if (!strcmp(name, test->fields[i].name))
			return test->fields[i].body;
	return NULL;
}

ErrorID parseTest(const char *src, NojaTest *test)
{
	NojaTestScanner scanner = {.src=src, .len=strlen(src), .cur=0};

	test->fields = test->fields_maybe;
	test->fields_count = 0;
	test->fields_capacity = sizeof(test->fields_maybe)/sizeof(test->fields_maybe[0]);
		
	bool done = false;
	do {

		if (test->fields_count == test->fields_capacity)
			if (!growFieldsArray(test)) {
				freeTest(test);
				return ErrorID_OUTOFMEMORY;
			}

		NojaTestField *field = test->fields + test->fields_count;

		ErrorID error = parseField(&scanner, field);
		switch (error) {
			
			case ErrorID_VOID:
			// Before acknowledging the insertion
			// by incrementing the field counter,
			// check that the new field doesn't have
			// a duplicate name
			if (queryTest(test, field->name))
				return ErrorID_DUPLFIELD;
			test->fields_count++;
			break;

			case ErrorID_BADSYNTAX_NOFIELD:
			done = true;
			break;

			default:
			freeTest(test); 
			return error;
		}
	} while (!done);
	return ErrorID_VOID;
}