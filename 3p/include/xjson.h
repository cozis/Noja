
#ifndef XJSON_H
#define XJSON_H

#include <stdarg.h>
#include <stdio.h>

enum {
	xj_INT       = (1 << 0),
	xj_BOOL      = (1 << 1),
	xj_NULL      = (1 << 2),
	xj_FLOAT     = (1 << 3),
	xj_ARRAY     = (1 << 4),
	xj_OBJECT    = (1 << 5),
	xj_STRING    = (1 << 6),
	xj_BIG_INT   = (1 << 7),
	xj_BIG_FLOAT = (1 << 8),
};

typedef enum {

	xj_E0000 = 0, // No error.


	// --------------------------- //
	// --- Internal errors ------- //
	// --------------------------- //
	
	xj_E0001, // Maximum depth reached.
	xj_E0002, // Out of memory.
	xj_E0003, // Not implemented.


	// --------------------------- //
	// --- Input errors ---------- //
	// --------------------------- //

	// (Unexpexted end of source inside object) //
	
	xj_E1000, 	// End of source inside object, 
				// right after the starting {
	
	xj_E1001, 	// End of source inside object, 
				// right after a key string. Was 
				// expected :
	
	xj_E1002, 	// End of source inside object, 
				// right after a key-value separator :. 
				// Was expected a value expression
	
	xj_E1003, 	// End of source inside object, 
				// right after value expression. 
				// Were expected , or }

	xj_E1004, 	// End of source inside object, 
				// right after value-key separator ,. 
				// Was expected a key string

	// (Unexpected end of source inside array) //

	xj_E1100, 	// End of source inside array, 
				// right after the starting [
	
	xj_E1101, 	// End of source inside array, 
				// right after a value expression 
	
	xj_E1102, 	// End of source inside array, 
				// right after value separator ,. 
				// Was expected a new value 
				// expression

	// (Unexpected end of source inside number) //
	
	xj_E1200, 	// End of source inside numeric 
				// expression, right after minus 
				// sign. Was expected a digit

	// (Unexpected end of source inside string) //
	xj_E1300, // End of source inside string expression
	xj_E1301, // End of source inside string expression, right after a backslash

	// (Unexpected character inside object) //
	xj_E2000, // Unexpected character in place of an object key string. Was expected "
	xj_E2001, // Unexpected character in place of an object key-value separator. Was expected :
	xj_E2002, // Unexpected character inside object, right after value expression. Were expected , or }

	// (Unexpected character inside array) //
	xj_E2100, // Unexpected character inside array, right after value expression. Were expecter ] or ,

	// (Unexpected character inside number) //
	xj_E2200, // Unexpected character inside number expression, right after minus sign. Was expected a digit after it

	// (Unexpected character inside string) //
	xj_E2300, // Unexpected character inside string expression, right after backslash. The only escapable characters are \, /, ", n, t, r, f, b and u.
	xj_E2301, // Unexpected character inside string expression, after \u. Was expected a hex digit

	xj_E2302, // Unexpected character inside string expression, right after unicode codepoint. Was expected an auxiliary codepoint
	xj_E2303, // Unexpected character inside string expression, right after \\. Was expected an auxiliary codepoint

	xj_E2304, // Invalid symbol
	xj_E2305, // Invalid auxiliary symbol
	xj_E2306, // Invalid codepoint

	// (Unexpected end of source inside value expression) //
	xj_E2400, // Unexpected character in place of a value expression. Were expected {, [, ", n, t, f or a digit.

	// (Other) //
	xj_E2500, // Unexpected character after root value expression
	xj_E2501, // Source string is empty (null pointer)	
	xj_E2502, // Source string is empty
	xj_E2503, // Duplicate key in object

	// Decodef errors
	xj_E3000, // Bad format

	// Query errors
	xj_E4000, // End of source after dot.
	xj_E4001, // Invalid character after dot.
	xj_E4002, // End of source after [.
	xj_E4003, // Float used as index.
	xj_E4004, // Index too big.
	xj_E4005, // Invalid character after [.
	xj_E4006, // End of source instead of closing ].
	xj_E4007, // Invalid character instead of closing ].
	xj_E4008, // Invalid character instead of dot and [.
	xj_E4009, // Missing node.

	// (i|f|b|s|a|o)Query errors
	xj_E4100, // Queried int is too big so it's not representable.
	xj_E4101, // Queried value is not an int.
	xj_E4102, // Queried float is too big, so it's not representable.
	xj_E4103, // Queried value is not a float.
	xj_E4104, // Queried value is not a string.
	xj_E4105, // Queried value is not a bool.
	xj_E4106, // Queried value is not an array.
	xj_E4107, // Queried value is not an object.

	xj_ECOUNT,
} xj_code;

typedef struct {
	
	const char *src;
	
	_Bool occurred;
	xj_code code;
	
	char 	text[128];
	
	int 	offset, 
			lineno;

	const char *cfile,
			   *cfunc;
	int 		cline;
} xj_error;

typedef void xj_alloc;

typedef struct xj_value xj_value;
struct xj_value {
	int offset, length, type, size;
	char *key;
	union {
		long long as_int;
		_Bool  	  as_bool;
		double 	  as_float;
		xj_value* as_array;
		xj_value* as_object;
		char*	  as_string;
	};
	xj_value *next;
};

xj_alloc* xj_init  (unsigned int default_chunk_size);
void      xj_free  (xj_alloc *alloc);
void*	  xj_malloc(xj_alloc *alloc, unsigned int size);

void  	  xj_set_malloc(void *(*malloc)(void*, unsigned int));
void*	  xj_sys_malloc(unsigned int size);
void  	  xj_set_free(void (*free)(void*, void*));
void  	  xj_sys_free(void *ptr);
void  	  xj_set_userp(void *userp);

int 	  xj_snprintf (char *buff, int buffsz, const char *fmt, ...);
int 	  xj_vsnprintf(char *buff, int buffsz, const char *fmt, va_list args);

xj_value* xj_query (xj_error *error, xj_value *root, const char *text, int len);
_Bool 	  xj_nquery(xj_error *error, xj_value *root, const char *text, int len);
int    	  xj_iquery(xj_error *error, xj_value *root, const char *text, int len);
double 	  xj_fquery(xj_error *error, xj_value *root, const char *text, int len);
_Bool 	  xj_bquery(xj_error *error, xj_value *root, const char *text, int len);
char 	 *xj_squery(xj_error *error, xj_value *root, const char *text, int len, int *count);
xj_value *xj_aquery(xj_error *error, xj_value *root, const char *text, int len, int *count);
xj_value *xj_oquery(xj_error *error, xj_value *root, const char *text, int len, int *count);

xj_value* xj_decode  (xj_error *error, xj_alloc **alloc, const char *text, int len);
xj_value* xj_decodef (xj_error *error, xj_alloc **alloc, const char *format, ...);
xj_value* xj_vdecodef(xj_error *error, xj_alloc **alloc, const char *format, va_list args);

char*  	  xj_encode (const xj_value *value, int *len);
char*	  xj_sencode(const char     *src, 	int *len);
char*	  xj_aencode(const xj_value *head, 	int *len);
char*	  xj_oencode(const xj_value *head, 	int *len);

char*  	  xj_encode2 (const xj_value *value, char *buff, unsigned int bufflen, int *len);
char*	  xj_sencode2(const char     *src, 	 char *buff, unsigned int bufflen, int *len);
char*	  xj_aencode2(const xj_value *head,  char *buff, unsigned int bufflen, int *len);
char*	  xj_oencode2(const xj_value *head,  char *buff, unsigned int bufflen, int *len);

_Bool 	  xj_compare(const xj_value *a, const xj_value *b);
xj_value* xj_get_by_name (const xj_value *parent, const char *name, int len);
xj_value* xj_get_by_index(const xj_value *parent, unsigned int index);
_Bool 	  xj_is_error_internal(const xj_error *error);
const char *xj_typename(int type);
void xj_stats(FILE *fp);
#endif