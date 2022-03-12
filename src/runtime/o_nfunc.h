#ifndef NOJAFUNC_H
#define NOJAFUNC_H
#include "runtime.h"
Object *Object_FromNativeFunction(Runtime *runtime, Object *(*callback)(Runtime*, Object**, unsigned int, Error*), int argc, Heap *heap, Error *error);
#endif