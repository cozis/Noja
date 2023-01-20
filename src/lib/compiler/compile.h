#ifndef COMPILE_H
#define COMPILE_H
#include "../executable.h"
#include "../utils/error.h"
#include "../utils/source.h"
Executable *compile(Source *src, Error *error, int *error_offset);
#endif /* COMPILE_H */
