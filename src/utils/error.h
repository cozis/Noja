#ifndef ERROR_H
#define ERROR_H
#include <stdarg.h>

typedef struct Error Error;

struct Error {
	void 		(*on_report)(Error *err);
	_Bool  		occurred, 
				internal, 
				truncated;
	int    		length;
	char*		message;
	char   		message2[256];
	const char *file,
			   *func;
	int 		line;
};

void 	Error_Init(Error *err);
void 	Error_Init2(Error *err, void (*on_report)(Error *err));
void 	Error_Free(Error *err);
#define Error_Report(err, internal, fmt, ...) _Error_Report(err, internal, __FILE__, __func__, __LINE__, fmt, ## __VA_ARGS__)
void   _Error_Report (Error *err, _Bool internal, const char *file, const char *func, int line, const char *fmt, ...);
void   _Error_Report2(Error *err, _Bool internal, const char *file, const char *func, int line, const char *fmt, va_list va);
#endif