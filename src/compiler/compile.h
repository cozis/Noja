#ifndef COMPILE_H
#define COMPILE_H
#include "../utils/error.h"
#include "../utils/bpalloc.h"
#include "../common/executable.h"
#include "AST.h"
Executable *compile(AST *ast, BPAlloc *alloc, Error *error);
#endif