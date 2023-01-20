#include <stdint.h>
#include <stdbool.h>
#include "../utils/error.h"
#include "../objects/objects.h"
#include "../runtime.h"

typedef struct {
	bool defined;
	union {
		struct {
			const char *data;
			size_t size;
		} as_string;
		struct {
			void  *data;
			size_t size;
		} as_buffer;
		bool    as_bool;
		int64_t as_int;
		double  as_float;
		FILE   *as_file;
	};
} ParsedArgument;

bool parseArgs(Error *error, Object **argv, unsigned int argc, ParsedArgument *pargs, const char *fmt);
int  returnValues (Error *error, Heap    *heap,    Object *rets[static MAX_RETS], const char *fmt, ...);
int  returnValues2(Error *error, Runtime *runtime, Object *rets[static MAX_RETS], const char *fmt, ...);
