#include <stdarg.h>
#include "utils/error.h"
#include "objects/objects.h"

Object *buildValue2(Heap *heap, Error *error, const char *fmt, va_list va);
Object *buildValue(Heap *heap, Error *error, const char *fmt, ...);