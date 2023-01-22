#ifndef TEST_FIELD_H
#define TEST_FIELD_H

#include <stddef.h>
#include "error.h"
#include "scanner.h"

typedef struct {
	char name_maybe[128];
	char body_maybe[512];
	char *name;
	char *body;
	size_t name_len;
	size_t body_len;
} NojaTestField;

void     freeField(NojaTestField *field);
ErrorID parseField(NojaTestScanner *scanner, NojaTestField *field);

#endif