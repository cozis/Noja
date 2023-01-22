#ifndef TEST_ERROR_H
#define TEST_ERROR_H

typedef enum {
	ErrorID_VOID,
	ErrorID_OUTOFMEMORY,
	ErrorID_DUPLFIELD,
	ErrorID_BADSYNTAX_NONAME,
	ErrorID_BADSYNTAX_NOAT,
	ErrorID_BADSYNTAX_NOFIELD,
} ErrorID;

const char *getErrorString(ErrorID error);
#endif