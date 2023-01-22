#include "error.h"

const char *getErrorString(ErrorID error)
{
	switch (error) {
		case ErrorID_VOID: return "No error";
		case ErrorID_OUTOFMEMORY: return "Out of memory";
		case ErrorID_DUPLFIELD: return "Duplicate field";
		case ErrorID_BADSYNTAX_NOAT: return "Missing '@' character";
		case ErrorID_BADSYNTAX_NONAME: return "Missing field name";
		case ErrorID_BADSYNTAX_NOFIELD: return "Missing field";
	}
	return "???";
}