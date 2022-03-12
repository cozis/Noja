#ifndef NOJAFUNC_H
#define NOJAFUNC_H
#include "runtime.h"
Object *Object_FromNojaFunction(Runtime *runtime, Executable *exe, int index, int argc, Object *closure, Heap *heap, Error *error);
#endif