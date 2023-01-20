#ifndef ASSEMBLE_H
#define ASSEMBLE_H
#include "../executable.h"
#include "../utils/error.h"
#include "../utils/source.h"
Executable *assemble(Source *src, Error *error, int *error_offset);
#endif /* ASSEMBLE_H */