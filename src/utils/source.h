#ifndef SOURCE_H
#define SOURCE_H
#include "error.h"
typedef struct xSource Source;
Source 		*Source_Copy(Source *s);
void 		 Source_Free(Source *s);
const char  *Source_GetName(const Source *s);
const char  *Source_GetBody(const Source *s);
unsigned int Source_GetSize(const Source *s);
Source 		*Source_FromFile(const char *file, Error *error);
Source 		*Source_FromString(const char *name, const char *body, int size, Error *error);
#endif