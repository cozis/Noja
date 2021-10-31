#include <assert.h>

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef NULL
#define NULL ((void*) 0)
#endif

#define UNREACHABLE assert(0);

#define membersizeof(type, member) (sizeof(((type*) 0)->member))