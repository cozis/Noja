#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "runtime/runtime.h"
#include "utils/defs.h"

static Object *select_(Object *self, Object *key, Heap *heap, Error *err);

static Object *bin_bind(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_listen(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_socket(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_accept(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_connect(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_inet_aton(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_inet_ntoa(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_htonl(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_ntohl(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_htons(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_ntohs(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_send(Runtime *runtime, Object **argv, unsigned int argc, Error *error);
static Object *bin_recv(Runtime *runtime, Object **argv, unsigned int argc, Error *error);


typedef struct {
	Object base;
	Runtime *runtime;
} NetworkBuiltinsMapOjbect;

static TypeObject t_builtins_map = {
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
			case PAIR(4, 's'):
			{
				if(!strcmp(s, "send"))
					return Object_FromNativeFunction(bm->runtime, bin_send, 3, heap, err);
				
				return NULL;
			}

			case PAIR(4, 'r'):
			{
				if(!strcmp(s, "recv"))
					return Object_FromNativeFunction(bm->runtime, bin_recv, 3, heap, err);
				
				return NULL;
			}

			case PAIR(10, 'I'):
			{
				if(!strcmp(s, "INADDR_ANY"))
					return Object_FromInt(INADDR_ANY, heap, err);
				
				return NULL;
			}

			case PAIR(6, 'a'):
			{
				if(!strcmp(s, "accept"))
					return Object_FromNativeFunction(bm->runtime, bin_accept, 2, heap, err);
				
				return NULL;
			}

			case PAIR(4, 'b'):
			{
				if(!strcmp(s, "bind"))
					return Object_FromNativeFunction(bm->runtime, bin_bind, 2, heap, err);
				
				return NULL;
			}

			case PAIR(6, 'l'):
			{
				if(!strcmp(s, "listen"))
					return Object_FromNativeFunction(bm->runtime, bin_listen, 2, heap, err);
				
				return NULL;
			}

			case PAIR(5, 'n'):
			{
				if(!strcmp(s, "ntohl"))
					return Object_FromNativeFunction(bm->runtime, bin_ntohl, 1, heap, err);

				if(!strcmp(s, "ntohs"))
					return Object_FromNativeFunction(bm->runtime, bin_ntohs, 1, heap, err);

				return NULL;
			}

			case PAIR(5, 'h'):
			{
				if(!strcmp(s, "htonl"))
					return Object_FromNativeFunction(bm->runtime, bin_htonl, 1, heap, err);

				if(!strcmp(s, "htons"))
					return Object_FromNativeFunction(bm->runtime, bin_htons, 1, heap, err);

				return NULL;
			}

			case PAIR(9, 'i'):
			{
				if(!strcmp(s, "inet_aton"))
					return Object_FromNativeFunction(bm->runtime, bin_inet_aton, 2, heap, err);

				if(!strcmp(s, "inet_ntoa"))
					return Object_FromNativeFunction(bm->runtime, bin_inet_ntoa, 1, heap, err);

				return NULL;
			}

			case PAIR(6, 's'):
			{
				if(!strcmp(s, "socket"))
					return Object_FromNativeFunction(bm->runtime, bin_socket, 3, heap, err);
				return NULL;
			}

			case PAIR(7, 'c'):
			{
				if(!strcmp(s, "connect"))
					return Object_FromNativeFunction(bm->runtime, bin_connect, 2, heap, err);
				return NULL;
			}

			case PAIR(8, 'S'):
			{
				if(!strcmp(s, "SOCK_RAW"))
					return Object_FromInt(SOCK_RAW, heap, err);
				
				if(!strcmp(s, "SOCK_RDM"))
					return Object_FromInt(SOCK_RDM, heap, err);
				
				return NULL;
			}

			case PAIR(10, 'S'):
			{
				if(!strcmp(s, "SOCK_DGRAM"))
					return Object_FromInt(SOCK_DGRAM, heap, err);
				
				return NULL;
			}

			case PAIR(11, 'S'):
			{
				if(!strcmp(s, "SOCK_STREAM"))
					return Object_FromInt(SOCK_STREAM, heap, err);
				
				if(!strcmp(s, "SOCK_PACKET"))
					return Object_FromInt(SOCK_PACKET, heap, err);
				
				return NULL;
			}

			case PAIR(12, 'S'):
			{
				if(!strcmp(s, "SOCK_CLOEXEC"))
					return Object_FromInt(SOCK_CLOEXEC, heap, err);
				
				return NULL;
			}

			case PAIR(13, 'S'):
			{
				if(!strcmp(s, "SOCK_NONBLOCK"))
					return Object_FromInt(SOCK_NONBLOCK, heap, err);
				
				return NULL;
			}

			case PAIR(14, 'S'):
			{
				if(!strcmp(s, "SOCK_SEQPACKET"))
					return Object_FromInt(SOCK_SEQPACKET, heap, err);
				
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

	return Object_FromInt(fd, Runtime_GetHeap(runtime), error);
}


static _Bool get_in_addr_from_map(Object *map, struct in_addr *addr, Heap *heap, Error *error)
{
	Object *o_key = Object_FromString("s_addr", -1, heap, error);
	if(o_key == NULL) return 0;

	Object *o_s_addr = Object_Select(map, o_key, heap, error);
	if(o_s_addr == NULL)
		{
			if(error->occurred == 0)
				Error_Report(error, 0, "Missing the \"s_addr\" key");
			return 0;
		}

	long long int s_addr = Object_ToInt(o_s_addr, error);
	if(error->occurred) return 0;

	if(s_addr < 0 || s_addr >= ULONG_MAX)
		{
			Error_Report(error, 0, "s_addr must be in range [0, %lu]", ULONG_MAX);
			return 0;
		}

	if(addr)
		addr->s_addr = s_addr;

	return 1;
}

static _Bool get_sockaddr_in_from_map(Object *map, struct sockaddr_in *addr, Heap *heap, Error *error)
{
	Object *o_key = Object_FromString("sin_family", -1, heap, error);
	if(o_key == NULL) return 0;

	Object *o_sin_family = Object_Select(map, o_key, heap, error);
	if(o_sin_family == NULL)
		{
			if(error->occurred == 0)
				Error_Report(error, 0, "Argument 2 is missing the \"sin_family\" key");
			return 0;
		}

	long long int sin_family = Object_ToInt(o_sin_family, error);
	if(error->occurred) return 0;

	if(sin_family < 0 || sin_family >= 65536)
		{
			Error_Report(error, 0, "sin_family must be in range [0, 65535]");
			return 0;
		}
	
	o_key = Object_FromString("sin_port", -1, heap, error);
	if(o_key == NULL) return 0;

	Object *o_sin_port = Object_Select(map, o_key, heap, error);
	if(o_sin_port == NULL)
		{
			if(error->occurred == 0)
				Error_Report(error, 0, "Argument 2 is missing the \"sin_port\" key");
			return 0;
		}

	long long int sin_port = Object_ToInt(o_sin_port, error);
	if(error->occurred) return 0;

	if(sin_port < 0 || sin_port >= 65536)
		{
			Error_Report(error, 0, "sin_port must be in range [0, 65535]");
			return 0;
		}

	o_key = Object_FromString("sin_addr", -1, heap, error);
	if(o_key == NULL) return 0;

	Object *o_sin_addr = Object_Select(map, o_key, heap, error);
	if(o_sin_addr == NULL)
		{
			if(error->occurred == 0)
				Error_Report(error, 0, "Argument 2 is missing the \"sin_addr\" key");
			return 0;
		}

	struct in_addr sin_addr;

	if(!get_in_addr_from_map(o_sin_addr, &sin_addr, heap, error))
		return 0;

	if(addr)
		{
			memset(addr, 0, sizeof(struct sockaddr_in));
			addr->sin_family = sin_family;
			addr->sin_port   = sin_port;
			addr->sin_addr   = sin_addr;
		}
	return 1;
}

static Object *bin_connect(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 2);

	Heap *heap = Runtime_GetHeap(runtime);

	int sockfd;
	struct sockaddr_in addr;

	sockfd = Object_ToInt(argv[0], error);
	if(error->occurred == 1) return NULL;

	if(!get_sockaddr_in_from_map(argv[1], &addr, heap, error))
		return NULL;

	int r = connect(sockfd, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));

	return Object_FromInt(r, Runtime_GetHeap(runtime), error);
}

static Object *bin_inet_aton(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 2);

	Heap *heap = Runtime_GetHeap(runtime);

	const char *s = Object_ToString(argv[0], NULL, heap, error);
	if(s == NULL) return NULL;

	// Note that the string must be zero-terminated.

	struct in_addr addr;

	int r = inet_aton(s, &addr);

	if(r != 0)
		{
			// inet_aton succeded.

			Object *o_key = Object_FromString("s_addr", -1, heap, error);
			if(o_key == NULL) return NULL;

			Object *o_val = Object_FromInt(addr.s_addr, heap, error);
			if(o_val == NULL) return NULL;

			if(!Object_Insert(argv[1], o_key, o_val, heap, error))
				return NULL;
		}

	return Object_FromInt(r, heap, error);
}

static Object *bin_inet_ntoa(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	Heap *heap = Runtime_GetHeap(runtime);

	struct in_addr in_addr;

	if(!get_in_addr_from_map(argv[0], &in_addr, heap, error))
		return NULL;

	const char *addr = inet_ntoa(in_addr);
	assert(addr != NULL);

	return Object_FromString(addr, -1, heap, error);
}

static Object *bin_htonl(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	Heap *heap = Runtime_GetHeap(runtime);


	uint32_t r, p;

	p = Object_ToInt(argv[0], error);
	if(error->occurred) return NULL;

	r = htonl(p);

	return Object_FromInt(r, heap, error);
}

static Object *bin_ntohl(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	Heap *heap = Runtime_GetHeap(runtime);


	uint32_t r, p;

	p = Object_ToInt(argv[0], error);
	if(error->occurred) return NULL;

	r = ntohl(p);

	return Object_FromInt(r, heap, error);
}

static Object *bin_ntohs(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	Heap *heap = Runtime_GetHeap(runtime);

	int p = Object_ToInt(argv[0], error);
	if(error->occurred) return NULL;

	if(p < 0 || p > 65535)
		{
			Error_Report(error, 0, "Argument is not in range [0, 65535]");
			return NULL;
		}

	uint16_t r = ntohs(p);

	return Object_FromInt(r, heap, error);
}

static Object *bin_htons(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 1);

	Heap *heap = Runtime_GetHeap(runtime);

	int p = Object_ToInt(argv[0], error);
	if(error->occurred) return NULL;

	if(p < 0 || p > 65535)
		{
			Error_Report(error, 0, "Argument is not in range [0, 65535]");
			return NULL;
		}

	uint16_t r = htons(p);

	return Object_FromInt(r, heap, error);
}

static Object *bin_bind(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 2);

	Heap *heap = Runtime_GetHeap(runtime);

	int sockfd;
	struct sockaddr_in addr;

	sockfd = Object_ToInt(argv[0], error);
	if(error->occurred) return NULL;

	if(!get_sockaddr_in_from_map(argv[1], &addr, heap, error))
		return 0;

	int r = bind(sockfd, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));

	return Object_FromInt(r, heap, error);
}

static Object *bin_listen(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 2);

	Heap *heap = Runtime_GetHeap(runtime);

	int sockfd, backlog;

	sockfd = Object_ToInt(argv[0], error);
	if(error->occurred) return NULL;

	backlog = Object_ToInt(argv[1], error);
	if(error->occurred) return NULL;

	int r = listen(sockfd, backlog);

	return Object_FromInt(r, heap, error);
}

static Object *bin_accept(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 2);

	Heap *heap = Runtime_GetHeap(runtime);

	long long int sockfd = Object_ToInt(argv[0], error);
	if(error->occurred) return NULL;

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	int r = accept(sockfd, (struct sockaddr*) &addr, &len);

	if(r >= 0) {

		// accept succeded.

		Object *o_key, 
		       *o_sin_family, 
		       *o_sin_addr, 
		       *o_sin_port,
		       *o_s_addr;

		o_sin_family = Object_FromInt(addr.sin_family, heap, error);
		if(error->occurred) return NULL;

		o_sin_port = Object_FromInt(addr.sin_port, heap, error);
		if(error->occurred) return NULL;

		o_sin_addr = Object_NewMap(1, heap, error);
		if(error->occurred) return NULL;

		o_s_addr = Object_FromInt(addr.sin_addr.s_addr, heap, error);
		if(error->occurred) return NULL;

		o_key = Object_FromString("s_addr", -1, heap, error);
		if(error->occurred) return NULL;

		if(!Object_Insert(o_sin_addr, o_key, o_s_addr, heap, error))
			return NULL;

		o_key = Object_FromString("sin_family", -1, heap, error);
		if(error->occurred) return NULL;

		if(!Object_Insert(argv[1], o_key, o_sin_family, heap, error))
			return NULL;

		o_key = Object_FromString("sin_port", -1, heap, error);
		if(error->occurred) return NULL;

		if(!Object_Insert(argv[1], o_key, o_sin_port, heap, error))
			return NULL;

		o_key = Object_FromString("sin_addr", -1, heap, error);
		if(error->occurred) return NULL;

		if(!Object_Insert(argv[1], o_key, o_sin_addr, heap, error))
			return NULL;
	}

	return Object_FromInt(r, heap, error);
}

static Object *bin_send(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 3);

	Heap *heap = Runtime_GetHeap(runtime);

	int sockfd = Object_ToInt(argv[0], error);
	if(error->occurred) return NULL;

	int   size;
	void *addr = Object_GetBufferAddrAndSize(argv[1], &size, error);
	if(addr == NULL) return NULL;

	int flags = Object_ToInt(argv[2], error);
	if(error->occurred) return NULL;

	ssize_t r = send(sockfd, addr, size, flags);

	return Object_FromInt(r, heap, error);
}

static Object *bin_recv(Runtime *runtime, Object **argv, unsigned int argc, Error *error)
{
	assert(argc == 3);

	Heap *heap = Runtime_GetHeap(runtime);

	int sockfd = Object_ToInt(argv[0], error);
	if(error->occurred) return NULL;

	int   size;
	void *addr = Object_GetBufferAddrAndSize(argv[1], &size, error);
	if(addr == NULL) return NULL;

	int flags = Object_ToInt(argv[2], error);
	if(error->occurred) return NULL;

	ssize_t r = recv(sockfd, addr, size, flags);

	return Object_FromInt(r, heap, error);
}