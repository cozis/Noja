#ifndef RUNTIME_ERROR_H
#define RUNTIME_ERROR_H
#include "../utils/error.h"
#include "runtime.h"

typedef struct {
	Error base;
	Runtime *runtime;
	Snapshot *snapshot;
} RuntimeError;

void RuntimeError_Init(RuntimeError *error, Runtime *runtime);
void RuntimeError_Free(RuntimeError *error);
#endif