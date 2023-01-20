#include "run.h"
#include "builtins_api.h"
#include "../builtins/basic.h"

bool Runtime_plugBuiltinsFromStaticMap(Runtime *runtime, StaticMapSlot *bin_table,  void (*bin_table_constructor)(StaticMapSlot*), Error *error)
{
	Object *object = Object_NewStaticMap(bin_table, bin_table_constructor, runtime, error);
	if (object == NULL)
		return false;
	return Runtime_plugBuiltins(runtime, object, error);
}

bool Runtime_plugBuiltinsFromSource(Runtime *runtime, Source *source, Error *error)
{
    Object *rets[8];
    int retc = runSource(runtime, source, rets, error);
    if(retc < 0)
        return false;
    if (retc == 0)
        return true;
    return Runtime_plugBuiltins(runtime, rets[0], error);
}

bool Runtime_plugBuiltinsFromFile(Runtime *runtime, const char *file, Error *error)
{
	Source *source = Source_FromFile(file, error);
    if (source == NULL)
        return false;
    
    bool result = Runtime_plugBuiltinsFromSource(runtime, source, error);

    Source_Free(source);
    return result;
}

bool Runtime_plugBuiltinsFromString(Runtime *runtime, const char *string, Error *error)
{
	Source *source = Source_FromString("<prelude>", string, -1, error);
    if (source == NULL)
        return false;
    
    bool result = Runtime_plugBuiltinsFromSource(runtime, source, error);

    Source_Free(source);
    return result;
}

bool Runtime_plugDefaultBuiltins(Runtime *runtime, Error *error)
{
	extern char start_noja[];
    return Runtime_plugBuiltinsFromStaticMap(runtime, bins_basic, bins_basic_init, error)
		&& Runtime_plugBuiltinsFromString(runtime, start_noja, error)
    	&& Runtime_plugBuiltinsFromString(runtime, "XXX=999;", error);
}