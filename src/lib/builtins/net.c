#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "net.h"
#include "utils.h"
#include "../utils/defs.h"

static int bin_socket(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	ASSERT(argc == 4);
	ParsedArgument pargs[4];
	if (!parseArgs(error, argv, argc, pargs, "iii?b"))
		return -1;

	int domain = pargs[0].as_int;
	int   type = pargs[1].as_int;
	int  proto = pargs[2].as_int;
	bool reuse = pargs[3].defined ? pargs[3].as_bool : false;

	int fd = socket(domain, type, proto);
		
	if (fd < 0)
		return returnValues2(error, runtime, rets, "ns", strerror(errno));
	
	if (reuse) {
		int value = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)
			return returnValues2(error, runtime, rets, "ns", strerror(errno));
	}

	return returnValues2(error, runtime, rets, "i", fd);
}

static int bin_bind(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	// 0: fd
	// 1: family
	// 2: port (family=AF_INET)
	// 3: addr (family=AF_INET)

	ASSERT(argc == 4);
	ParsedArgument pargs[4];
	if (!parseArgs(error, argv, argc, pargs, "iii?s"))
		return -1;

	int     fd = pargs[0].as_int;
	int family = pargs[1].as_int;

	if (family != AF_INET)
		return returnValues2(error, runtime, rets, "ns", "Invalid family (only AF_INET is supported at the moment)");

	int   port = pargs[2].as_int;
	const char *addr = pargs[3].defined ? pargs[3].as_string.data : NULL;

	struct in_addr parsed_addr;
	if (addr == NULL)
		parsed_addr.s_addr = INADDR_ANY;
	else {
		int res = inet_pton(family, addr, &parsed_addr);
		if (res == 0) return returnValues2(error, runtime, rets, "ns", "Invalid address");
		if (res  < 0) return returnValues2(error, runtime, rets, "ns", "Invalid family");
	}

	struct sockaddr_in buff;
	buff.sin_family = family;
	buff.sin_port   = htons(port);
	buff.sin_addr   = parsed_addr;
	if (bind(fd, (struct sockaddr*) &buff, sizeof(buff)) < 0)
		return returnValues2(error, runtime, rets, "s", strerror(errno));

	return returnValues2(error, runtime, rets, "n");
}

static int bin_listen(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	// 0: fd
	// 1: backlog

	ASSERT(argc == 2);
	ParsedArgument pargs[2];
	if (!parseArgs(error, argv, argc, pargs, "ii"))
		return -1;

	int fd      = pargs[0].as_int;
	int backlog = pargs[1].as_int;

	if (listen(fd, backlog) < 0)
		return returnValues2(error, runtime, rets, "s", strerror(errno));
	return returnValues2(error, runtime, rets, "n");
}

static int bin_recv(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	// 0: fd
	// 1: buffer
	// 2: count

	ASSERT(argc == 3);
	ParsedArgument pargs[3];
	if (!parseArgs(error, argv, argc, pargs, "iB?i"))
		return -1;

	int        fd = pargs[0].as_int;
	void  *dstptr = pargs[1].as_buffer.data;
	size_t dstlen = pargs[1].as_buffer.size;

	size_t count;
	if (pargs[2].defined) {
		int n = pargs[2].as_int;
		if (n < 0)
			return returnValues2(error, runtime, rets, "ns", "Invalid negative count");
		count = MIN((size_t) n, dstlen);
	} else
		count = dstlen;
	
	ssize_t res = recv(fd, dstptr, count, 0);
	if (res < 0)
		return returnValues2(error, runtime, rets, "ns", strerror(errno));
	return returnValues2(error, runtime, rets, "i", res);
}

static int bin_send(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	// 0: fd
	// 1: buffer
	// 2: count

	ASSERT(argc == 3);
	ParsedArgument pargs[3];
	if (!parseArgs(error, argv, argc, pargs, "iB?i"))
		return -1;

	int        fd = pargs[0].as_int;
	void  *srcptr = pargs[1].as_buffer.data;
	size_t srclen = pargs[1].as_buffer.size;

	size_t count;
	if (pargs[2].defined) {
		int n = pargs[2].as_int;
		if (n < 0)
			return returnValues2(error, runtime, rets, "ns", "Invalid negative count");
		count = MIN((size_t) n, srclen);
	} else
		count = srclen;
	
	ssize_t res = send(fd, srcptr, count, 0);
	if (res < 0)
		return returnValues2(error, runtime, rets, "ns", strerror(errno));
	return returnValues2(error, runtime, rets, "i", res);
}

static int bin_accept(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	// 0: fd

	ASSERT(argc == 1);
	ParsedArgument pargs[1];
	if (!parseArgs(error, argv, argc, pargs, "i"))
		return -1;

	int fd = pargs[0].as_int;
	
	struct sockaddr_in new_addr;
	socklen_t new_addr_size;
	int new_fd = accept(fd, (struct sockaddr*) &new_addr, &new_addr_size);
	if (new_fd < 0)
		return returnValues2(error, runtime, rets, "ns", strerror(errno));
	ASSERT(new_addr.sin_family == AF_INET);
	ASSERT(new_addr_size == sizeof(struct sockaddr_in));

	return returnValues2(error, runtime, rets, "iii", new_fd, new_addr.sin_addr.s_addr, new_addr.sin_port);
}


static int bin_close(Runtime *runtime, Object **argv, unsigned int argc, Object *rets[static MAX_RETS], Error *error)
{
	// 0: fd

	ASSERT(argc == 1);
	ParsedArgument pargs[1];
	if (!parseArgs(error, argv, argc, pargs, "i"))
		return -1;

	int fd = pargs[0].as_int;
	int res = close(fd);
	return returnValues2(error, runtime, rets, "i", res);
}

StaticMapSlot bins_net[] = {
	{ "AF_INET", SM_INT, .as_int = AF_INET, },
	{ "SOCK_STREAM", SM_INT, .as_int = SOCK_STREAM, },
	{ "SOCK_DGRAM",  SM_INT, .as_int = SOCK_DGRAM,  },
	{ "socket",  SM_FUNCT, .as_funct = bin_socket, .argc = 4, },
	{ "bind",    SM_FUNCT, .as_funct = bin_bind,   .argc = 4, },
	{ "listen",  SM_FUNCT, .as_funct = bin_listen, .argc = 2, },
	{ "accept",  SM_FUNCT, .as_funct = bin_accept, .argc = 1, },
	{ "recv",    SM_FUNCT, .as_funct = bin_recv,   .argc = 3, },
	{ "send",    SM_FUNCT, .as_funct = bin_send,   .argc = 3, },
	{ "close",   SM_FUNCT, .as_funct = bin_close,  .argc = 1, },
	{ NULL, SM_END, {}, {} },
};
