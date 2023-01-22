#include "field.h"
#include "error.h"

typedef struct {
	NojaTestField fields_maybe[8];
	NojaTestField *fields;
	size_t fields_count;
	size_t fields_capacity;
} NojaTest;
ErrorID parseTest(const char *src, NojaTest *test);
const char *queryTest(NojaTest *test, const char *name);
void freeTest(NojaTest *test);
