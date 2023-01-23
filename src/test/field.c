#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "field.h"

static void freeFieldName(NojaTestField *field)
{
	if (field->name != field->name_maybe)
		free(field->name);
}

static void freeFieldBody(NojaTestField *field)
{
	if (field->body != field->body_maybe)
		free(field->body);
}

void freeField(NojaTestField *field)
{
	freeFieldName(field);
	freeFieldBody(field);
}

static ErrorID setFieldName(NojaTestField *field, const char *str, size_t len)
{
	if (len < sizeof(field->name_maybe))
		field->name = field->name_maybe;
	else {
		field->name = malloc(len+1);
		if (field->name == NULL)
			return ErrorID_OUTOFMEMORY;
	}
	memcpy(field->name, str, len);
	field->name[len] = '\0';
	return ErrorID_VOID;
}

static ErrorID setFieldBody(NojaTestField *field, const char *str, size_t len)
{
	if (len < sizeof(field->body_maybe))
		field->body = field->body_maybe;
	else {
		field->body = malloc(len+1);
		if (field->body == NULL)
			return ErrorID_OUTOFMEMORY;
	}
	memcpy(field->body, str, len);
	field->body[len] = '\0';
	return ErrorID_VOID;
}

static int isFieldNameCharacter(int c)
{
	return isalpha(c) || c == '_';
}

static ErrorID parseFieldName(NojaTestScanner *scanner, NojaTestField *field)
{
	size_t offset;
	size_t length;

	offset = scanner->cur;
	consumeCharactersThat(scanner, isFieldNameCharacter);
	length = scanner->cur - offset;
	
	if (length == 0)
		return ErrorID_BADSYNTAX_NONAME; // Missing field name token after '@'

	ErrorID error = setFieldName(field, scanner->src + offset, length);
	if (error != ErrorID_VOID) return error;

	return ErrorID_VOID;
}

static bool parseFieldBody(NojaTestScanner *scanner, NojaTestField *field)
{
	size_t offset;
	size_t length;

	consumeCharactersThat(scanner, isspace);
	
	bool no_term;
	char term;

	if (ifFollowsConsumeCharacter(scanner, '[')) {
		no_term = false;
		term = ']';
	} else if (ifFollowsConsumeCharacter(scanner, '{')) {
		no_term = false;
		term = '}';
	} else
		no_term = true;
	
	offset = scanner->cur;
	while (!noMoreCharacters(scanner) && !followsCharacter(scanner, '@') && (no_term || !followsCharacter(scanner, term)))
		scanner->cur++;
	length = scanner->cur - offset;
	if (!no_term)
		ifFollowsConsumeCharacter(scanner, term);

	return setFieldBody(field, scanner->src + offset, length);
}

ErrorID findField(NojaTestScanner *scanner)
{
	consumeCharactersThat(scanner, isspace);
	bool at_follows = ifFollowsConsumeCharacter(scanner, '@');
	if (!at_follows) {
		if (noMoreCharacters(scanner))
			return ErrorID_BADSYNTAX_NOFIELD;
		return ErrorID_BADSYNTAX_NOAT;
	}
	return ErrorID_VOID;
}

ErrorID parseField(NojaTestScanner *scanner, NojaTestField *field)
{
	ErrorID error;

	error = findField(scanner);
	if (error != ErrorID_VOID)
		return error;

	error = parseFieldName(scanner, field);
	if (error != ErrorID_VOID) 
		return error;

	error = parseFieldBody(scanner, field);
	if (error != ErrorID_VOID) {
		freeFieldName(field);
		return error;
	}

	return ErrorID_VOID;
}
