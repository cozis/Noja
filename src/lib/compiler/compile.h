#ifndef COMPILE_H
#define COMPILE_H
#include "../utils/error.h"
#include "../utils/source.h"
#include "../common/executable.h"
typedef enum {
    CompilationErrorType_SYNTAX,
    CompilationErrorType_SEMANTIC,
    CompilationErrorType_INTERNAL,
} CompilationErrorType;
Executable *compile(Source *src, Error *error, CompilationErrorType *errtyp);
#endif /* COMPILE_H */