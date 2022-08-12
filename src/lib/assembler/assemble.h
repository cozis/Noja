#ifndef ASSEMBLE_H
#define ASSEMBLE_H
#include "../utils/error.h"
#include "../utils/source.h"
#include "../common/executable.h"
Executable *assemble(Source *src, Error *error);
#endif /* ASSEMBLE_H */