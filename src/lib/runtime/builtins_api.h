#include "runtime.h"
bool Runtime_plugDefaultBuiltins(Runtime *runtime, Error *error);
bool Runtime_plugBuiltinsFromString(Runtime *runtime, const char *string, Error *error);
bool Runtime_plugBuiltinsFromFile(Runtime *runtime, const char *file, Error *error);
bool Runtime_plugBuiltinsFromStaticMap(Runtime *runtime, StaticMapSlot *bin_table,  void (*bin_table_constructor)(StaticMapSlot*), Error *error);
bool Runtime_plugBuiltinsFromSource(Runtime *runtime, Source *source, Error *error);
