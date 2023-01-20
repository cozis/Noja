#include "runtime.h"
int  runSource(Runtime *runtime, Source *source, Object *rets[static MAX_RETS], Error *error);
int  runBytecodeSource(Runtime *runtime, Source *source, Object *rets[static MAX_RETS], Error *error);
bool runFile(Runtime *runtime, const char *file, Error *error);
bool runString(Runtime *runtime, const char *string, Error *error);
bool runBytecodeFile(Runtime *runtime, const char *file, Error *error);
bool runBytecodeString(Runtime *runtime, const char *string, Error *error);
int  runFileEx(Runtime *runtime, const char *file, Object *rets[static MAX_RETS], Error *error);
int  runStringEx(Runtime *runtime, const char *name, const char *string, Object *rets[static MAX_RETS], Error *error);
int  runBytecodeFileEx(Runtime *runtime, const char *file, Object *rets[static MAX_RETS], Error *error);
int  runBytecodeStringEx(Runtime *runtime, const char *name, const char *string, Object *rets[static MAX_RETS], Error *error);
int  runFileRelativeToScript(Runtime *runtime, const char *file, Object *rets[static MAX_RETS], Error *error);
