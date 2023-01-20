#ifndef COMPILE_H
#define COMPILE_H
#include "../utils/error.h"
#include "../utils/source.h"
#include "../common/executable.h"
Executable *compile(Source *src, Error *error, int *error_offset);
#endif /* COMPILE_H */