#ifndef PARSE_H
#define PARSE_H
#include "../utils/bpalloc.h"
#include "../utils/source.h"
#include "../utils/error.h"
#include "AST.h"
AST *parse(Source *src, BPAlloc *alloc, Error *error);
#endif