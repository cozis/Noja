#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include "runtime/runtime.h"
#include "utils/defs.h"

static Object *select_(Object *self, Object *key, Heap *heap, Error *err);

static Object *bin_socket(Runtime *runtime, Object **argv, unsigned int argc, Error *error);

typedef struct {
	Object base;
	Runtime *runtime;
} NetworkBuiltinsMapOjbect;

static const Type t_builtins_map = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "network builtins map",
	.size = sizeof(NetworkBuiltinsMapOjbect),
	.select = select_,
};

static Object *select_(Object *self, Object *key, Heap *heap, Error *err)
{
	NetworkBuiltinsMapOjbect *bm = (NetworkBuiltinsMapOjbect*) self;

	if(!Object_IsString(key))
		{
			Error_Report(err, 0, "Non string key");
			return NULL;
		}

	int         n;
	const char *s;

	s = Object_ToString(key, &n, heap, err);

	if(s == NULL)
		return NULL;

	#define PAIR(p, q) \
		(((uint64_t) (p) << 32) | (uint32_t) (q))

	switch(PAIR(n, s[0]))
		{
			case PAIR(6, 's'):
			{
				if(!strcmp(s, "socket"))
					return Object_FromNativeFunction(bm->runtime, bin_socket, 3, heap, err);
				return NULL;
			}

			case PAIR(5, 'A'):
			{
				if(s[1] == 'F' && s[2] == '_')
					{
						if(!strcmp(s+3, "IB"))
							return Object_FromInt(AF_IB, heap, err);
					}
				return NULL;
			}

			case PAIR(6, 'A'):
			{
				if(s[1] == 'F' && s[2] == '_')
					{
						if(!strcmp(s+3, "IPX"))
							return Object_FromInt(AF_IPX, heap, err);

						if(!strcmp(s+3, "X25"))
							return Object_FromInt(AF_X25, heap, err);
							
						if(!strcmp(s+3, "KEY"))
							return Object_FromInt(AF_KEY, heap, err);

						if(!strcmp(s+3, "RDS"))
							return Object_FromInt(AF_RDS, heap, err);

						if(!strcmp(s+3, "LLC"))
							return Object_FromInt(AF_LLC, heap, err);
						
						if(!strcmp(s+3, "CAN"))
							return Object_FromInt(AF_CAN, heap, err);
						
						if(!strcmp(s+3, "ALG"))
							return Object_FromInt(AF_ALG, heap, err);
					
						if(!strcmp(s+3, "KCM"))
							return Object_FromInt(AF_KCM, heap, err);
					
						if(!strcmp(s+3, "XDP"))
							return Object_FromInt(AF_XDP, heap, err);
					}
				return NULL;
			}

			case PAIR(7, 'A'):
			{
				if(s[1] == 'F' && s[2] == '_')
					{
						if(!strcmp(s+3, "UNIX"))
							return Object_FromInt(AF_UNIX, heap, err);

						if(!strcmp(s+3, "INET"))
							return Object_FromInt(AF_INET, heap, err);
							
						if(!strcmp(s+3, "AX25"))
							return Object_FromInt(AF_AX25, heap, err);

						if(!strcmp(s+3, "MPLS"))
							return Object_FromInt(AF_MPLS, heap, err);

						if(!strcmp(s+3, "TIPC"))
							return Object_FromInt(AF_TIPC, heap, err);
					}
				return NULL;
			}

			case PAIR(8, 'A'):
			{
				if(s[1] == 'F' && s[2] == '_')
					{
						if(!strcmp(s+3, "LOCAL"))
							return Object_FromInt(AF_LOCAL, heap, err);

						if(!strcmp(s+3, "INET6"))
							return Object_FromInt(AF_INET6, heap, err);
							
						if(!strcmp(s+3, "PPPOX"))
							return Object_FromInt(AF_PPPOX, heap, err);

						if(!strcmp(s+3, "VSOCK"))
							return Object_FromInt(AF_VSOCK, heap, err);
					}
				return NULL;
			}

			case PAIR(9, 'A'):
			{
				if(s[1] == 'F' && s[2] == '_')
					{
						if(!strcmp(s+3, "DECnet"))
							return Object_FromInt(AF_DECnet, heap, err);

						if(!strcmp(s+3, "PACKET"))
							return Object_FromInt(AF_PACKET, heap, err);
					}
				return NULL;
			}

			case PAIR(10, 'A'):
			{
				if(s[1] == 'F' && s[2] == '_')
					{
						if(!strcmp(s+3, "NETLINK"))
							return Object_FromInt(AF_NETLINK, heap, err);
					}
				return NULL;
			}

			case PAIR(12, 'A'):
			{
				if(s[1] == 'F' && s[2] == '_')
					{
						if(!strcmp(s+3, "BLUETOOTH"))
							return Object_FromInt(AF_BLUETOOTH, heap, err);
					
						if(!strcmp(s+3, "APPLETALK"))
							return Object_FromInt(AF_APPLETALK, heap, err);
					}
				return NULL;
			}
		}

	// Not found.
	return NULL;
}

Object *Object_NewNetworkBuiltinsMap(Runtime *runtime, Heap *heap, Error *err)
{
	NetworkBuiltinsMapOjbect *bm = (NetworkBuiltinsMapOjbect*) Heap_Malloc(heap, &t_builtins_map, err);

	if(bm == NULL)
		return NULL;

	bm->runtime = runtime;
	
	return (Object*) bm;
}

static Object *bin_socket(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 3);

	int domain, type, protocol;

	domain = Object_ToInt(argv[0], error);
	if(error->occurred == 1) return NULL;

	type = Object_ToInt(argv[1], error);
	if(error->occurred == 1) return NULL;
	
	protocol = Object_ToInt(argv[2], error);
	if(error->occurred == 1) return NULL;

	int fd = socket(domain, type, protocol);

	if(fd < 0)
		{
			Error_Report(error, 0, "Failed to create socket (%s)", strerror(errno));
			return NULL;
		}

	return Object_FromInt(fd, Runtime_GetHeap(runtime), error);
}